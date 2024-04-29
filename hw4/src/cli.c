/*
 * Legion: Command-line interface
 */
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "legion.h"

#include "split.h"
#include "daemon.h"

#define HELP_MESSAGE "Available commands:\n\
help(0 args) Print this help message\n\
quit(0 args) Quit the program\n\
register (0 args) Register a daemon\n\
unregister(1 args) Unregister a daemon\n\
status(1 args) Show the status of a daemon\n\
status-all(0 args) Show the status of all daemons\n\
start(1 args) Start a daemon\n\
stop(1 args) Stop a daemon\n\
logrotate(1 args) Rotate log files for a daemon\n"

// --------------------------- START OF SIGNALS --------------------------- //

static volatile sig_atomic_t should_run = true; // status of the current command line prompt

static volatile sig_atomic_t recieved_alarm = false; // status of waiting for child sync message

static volatile sig_atomic_t recieved_child = false; // got SIGCHILD signal

/**
 * @brief set should_run to 0 indicating the command line prompt should stop.
 *
 */
void handle_SIGINT() {
    should_run = false;
}

void handle_SIGALRM() {
    recieved_alarm = true;
}

void hanle_SIGCHLD() {
    recieved_child = true;
}

// ---------------------------  END OF SIGNALS  --------------------------- //

static char *legion_argv = NULL; // malloc'd legion argument
static size_t legion_getline_buffer = 0; // for legion getline
static int legion_argc = 0; // for legion getline
static char **daemon_argv = NULL; // malloc'd daemon_argv
static int daemon_argc = 0; // daemon_argc

// --------------------------- START OF DYN MEM --------------------------- //

/**
 * @brief Free the 2D array which is the argv for the daemon
 *
 * @param arr
 *      the address of that 2D array
 * @param size
 *      the size of that 2D array
 */
void free_daemon_argv(char ***arr, int size) {
    for (int i = 0; i < size; i++) {
        free((*arr)[i]);
    }
    free((*arr)[size]);
    free(*arr);
}

/**
 * @brief Free a specific node within the daemon_list and update links accordingly.
 *
 * @param daemon
 *      the address of the daemon struct you're trying to free
 * @param prev_daemon
 *      the address of the previous daemon of the current daemon, NULL = head of the list which is the daemon_list itself.
 */
void free_daemon_node(sf_daemon **daemon, sf_daemon **prev_daemon) {
    char **arr = (*daemon)->daemon_argv;
    free_daemon_argv(&(arr), (*daemon)->daemon_argc);
    daemon_list.length--;
    if (*prev_daemon == NULL) {
        daemon_list.first = (*daemon)->links.next;
    } else {
        (*prev_daemon)->links.next = (*daemon)->links.next;
    }
    free((*daemon));
}

/**
 * @brief Free the entire list of the linked_list, also send SIGKILL to every process.
 *
 */
void free_daemon_list(sigset_t *mask_stop, sigset_t *mask_child, sigset_t *prev_mask) {
    sf_daemon *prev = NULL; // the freeing order will just free from left to right of linkedlist
    while (daemon_list.length > 0) {
        sf_daemon *current_free = daemon_list.first;
        if (current_free->status == status_active) {
            sf_stop(current_free->name, current_free->pid);
            current_free->status = status_stopping;
            if (sigprocmask(SIG_BLOCK, mask_child, prev_mask) < 0) {
                return;
            }
            kill(current_free->pid, SIGTERM);
            recieved_alarm = false;
            alarm(CHILD_TIMEOUT);
            sigsuspend(mask_stop);
            if (recieved_alarm) {
            kill:
                kill(current_free->pid, SIGKILL);
                sf_kill(current_free->name, current_free->pid);
            }
            int status = 0;
            pid_t test = waitpid(current_free->pid, &status, 0);
            alarm(0);
            if (test == -1) {
                goto kill;
            }
            if (sigprocmask(SIG_SETMASK, prev_mask, NULL) < 0) {
                return;
            }
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                sf_term(current_free->name, current_free->pid, exit_status);
                current_free->status = status_exited;
            } else if (WIFSIGNALED(status)) {
                int signal = WTERMSIG(status);
                sf_crash(current_free->name, current_free->pid, signal);
                current_free->status = status_crashed;
            }
        }
        free_daemon_node(&current_free, &prev);
    }
}

/**
 * @brief Free all dynamically allocated memory via this,
 * used some trickery with realloc almost everything such given if a pointer is NOT NULL
 * It is not Free. Will every dynamically allocated pointer to NULL after free for safety reason
 *
 */
void free_dynamic(sigset_t *mask_stop, sigset_t *mask_child, sigset_t *mask_prev) {
    if (legion_argv != NULL) {
        free(legion_argv);
        legion_argv = NULL;
    }
    if (daemon_argv != NULL) {
        free(daemon_argv);
        daemon_argv = NULL;
    }
    free_daemon_list(mask_stop, mask_child, mask_prev);
}

/**
 * @brief copy a String array (or formerly known as char**) to a new allocated memory.
 *
 * @param original
 *      the original char** array with the content to be copy over.
 * @param size
 *      the size of the original char** array.
 * @return char**
 *      the newly allocated malloc'd array.
 */
char **copy_daemon_argv(char **original, int size) {
    char **copy = (char **)malloc((size + 1) * sizeof(char *)); // Allocate memory for size + 1 pointers
    if (copy == NULL) {
        return NULL; // Allocation failure
    }
    for (int i = 0; i < size; i++) {
        int length = strlen(original[i]) + 1; // Add 1 for null terminator
        copy[i] = (char *)malloc(length * sizeof(char));
        if (copy[i] == NULL) {
            // Free memory allocated so far for safety
            for (int j = 0; j < i; j++) {
                free(copy[j]);
            }
            free(copy);
            return NULL; // allocaiton failure
        }
        strcpy(copy[i], original[i]);
    }
    copy[size] = NULL; // Set the last pointer to NULL (like the normal argv vector)
    return copy;
}

// ---------------------------  END OF DYN MEM  --------------------------- //

// -------------------------- START OF DAE_LIST --------------------------- //

/**
 * @brief Search for a daemon base of a target name String.
 *
 * @param search_name
 *      the name of a daemon you're looking for.
 * @return sf_daemon*
 *      return the memory adderss of that daemon if found. NULL = not found.
 */
sf_daemon *find_daemon(char *search_name) {
    sf_daemon *cursor = daemon_list.first;
    while (cursor) {
        if (strcmp(cursor->name, search_name) == 0) {
            return cursor;
        }
        cursor = cursor->links.next;
    }
    return NULL;
}

sf_daemon *find_daemon_id(pid_t pid) {
    sf_daemon *cursor = daemon_list.first;
    while (cursor) {
        if (cursor->pid == pid) {
            return cursor;
        }
        cursor = cursor->links.next;
    }
    return NULL;
}

/**
 * @brief Search for a daemon base of a target name String, also keep track of the previous node.
 *
 * @param search_name
 *      the name of a daemon you're looking for.
 * @param prev_tracker
 *      a pointer to store the address of the prev node.
 * @return sf_daemon*
 *      return the memory adderss of that daemon if found. NULL = not found.
 */
sf_daemon *find_daemon_2(char *search_name, sf_daemon **prev_tracker) {
    sf_daemon *cursor = daemon_list.first;
    sf_daemon *prev = NULL;
    while (cursor) {
        if (strcmp(cursor->name, search_name) == 0) {
            *prev_tracker = prev;
            return cursor;
        }
        prev = cursor;
        cursor = cursor->links.next;
    }
    *prev_tracker = NULL;
    return NULL;
}

// --------------------------- END OF DAE_LIST  --------------------------- //

// ------------------------ START OF STATUS PRINT ------------------------- //

/**
 * @brief print a singular status
 *
 * @param daemon
 *      NULL -> not a valid program, fall back to program name
 * @param program_name
 *      NULL -> not applicable (used for status-all), else use this fallback program name.
 * @param out
 *      the ouput File stream
 * @return
 *      -1 -> malloc'd failure, 0 normal
 */
int print_daemon_status(sf_daemon *daemon, char *program_name, FILE *out, bool *current_status) {
    size_t size = 0;
    char *str = NULL;
    if (daemon) {
        size = snprintf(NULL, 0, "%s\t%d\t%s", daemon->name, daemon->pid, daemon_state_names[daemon->status]) + 1;
        str = malloc(size);
        if (str == NULL) {
            fprintf(out, "ERROR: Dynamic allocation failed when attempted to format the daemon status.\n");
            return -1;
        }
        snprintf(str, size, "%s\t%d\t%s", daemon->name, daemon->pid, daemon_state_names[daemon->status]);
    } else {
        fprintf(out, "Daemon '%s' is not registered.\n", program_name);
        *current_status = false;
        return 0;
    }
    sf_status(str);
    fprintf(out, "%s\n", str);
    fflush(out);
    free(str);
    return 0;
}

/**
 * @brief print out all registered daemon in the linked_list
 *
 * @param out
 *      the ouput File stream
 * @return int
 *      -1 -> malloc'd failure, 0 normal
 */
int print_all_daemon_status(FILE *out) {
    sf_daemon *cursor = daemon_list.first;
    while (cursor) {
        bool temp = true;
        if (print_daemon_status(cursor, NULL, out, &temp)) {
            return -1;
        }
        cursor = cursor->links.next;
    }
    return 0;
}

/**
 * @brief Print an error message given output File stream, mssg and the address of the current_status, modify current_status to false.
 * Meant for Parent process
 *
 * @param out
 *      output File stream
 * @param mssg
 *      the mssg to print
 * @param curren_status
 *      the address of the current_status
 */
void print_error_parent(FILE *out, char *mssg, bool *curren_status) {
    fprintf(out, "%s\n", mssg);
    fflush(out);
    *curren_status = false;
}

/**
 * @brief Print an error message given output File stream, mssg (Meant for Child process)
 *
 * @param out
 * @param mssg
 */
void print_error_child(FILE *out, char *mssg) {
    fprintf(out, "%s\n", mssg);
    fflush(out);
}

/**
 * @brief use sf_error to print error mssg
 *
 * @param out
 *      the output to print an error message visible to user.
 * @param length_of_command
 *      length of the command for formatting (the command length)
 * @param argv
 *      the entire char* given from getline.
 */
void print_sf_error(FILE *out, size_t length_of_command, char *argv) {
    sf_error("Error executing command");
    fprintf(out, "ERROR: Error Executing command: '%.*s'\n", (int)length_of_command, argv);
}

// ------------------------- END OF STATUS PRINT  ------------------------- //

// -------------------------  START OF START CMD  ------------------------- //

/**
 * @brief Start the daemon, exit at anymore if function goes wrong. (REQUIRED STATIC GLOBAL VARIABLE)
 *
 * @param daemon
 *      the daemone we are trying to start.
 * @param current_status
 *      the current_status of the while_loop, will be modified and exit as soon as this is modified to false. (except in child where it will just abort())
 * @param out
 *      the FILE out stream to print error to.
 * @param child_mask
 *      a mask that mask out SIGCHLD specifically.
 * @param prev_mask
 *      a temporary mask storage to store pre_mask.
 */
void start_daemon(sf_daemon *daemon, bool *current_status, FILE *out, sigset_t *child_mask, sigset_t *prev_mask) {
    if (daemon->status != status_inactive) {
        fprintf(out, "ERROR: Daemon '%s' is not inactive.\n", daemon_argv[0]);
        fflush(out);
        *current_status = false;
        return;
    }
    daemon->status = status_starting;
    int newpipe[2];
    if (pipe(newpipe)) {
        daemon->status = status_inactive;
        print_error_parent(out, "ERROR: Cannot create pipe.", current_status);
        return;
    }
    sf_start(daemon->name);
    if (sigprocmask(SIG_BLOCK, child_mask, prev_mask) < 0) {
        print_error_parent(out, "ERROR: couldn't install a mask sigchld when about to start child process.", current_status);
        return;
    }
    pid_t pid = fork();
    if (pid < 0) { // fork failure
        sf_reset(daemon->name);
        daemon->status = status_inactive;
        print_error_parent(out, "ERROR: Cannot fork new process.", current_status);
        if (sigprocmask(SIG_SETMASK, prev_mask, NULL) < 0) {
            print_error_parent(out, "ERROR: couldn't unmask the mask sigchld after folk failed", current_status);
        }
        return;
    } else if (pid == 0) {
        if (sigprocmask(SIG_SETMASK, prev_mask, NULL) < 0) { // unmask the sigchld in child process
            print_error_child(out, "ERROR: couldn't unmask the mask sigchld of the child process.");
            abort();
        }
        // new process groupd id
        if (setpgid(0, 0) == -1) {
            print_error_child(out, "ERROR: cannot create new and make current process the leader of the new process group in the child process.");
            abort();
        }
        close(newpipe[0]); // close child read pipe as child is writing
        // need to dub2 and open the log file
        size_t log_file_dimension = snprintf(NULL, 0, "%s/%s.log.%d", LOGFILE_DIR, daemon->name, 0) + 1;
        char *log_file_path = malloc(log_file_dimension);
        if (log_file_path == NULL) {
            print_error_child(out, "ERROR: failed malloc'd when attempt to create the file path toward the corresponding log file in child process.");
            abort();
        }
        snprintf(log_file_path, log_file_dimension, "%s/%s.log.%d", LOGFILE_DIR, daemon->name, 0);
        int log_fd;
        if ((log_fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0777)) == -1) {
            print_error_child(out, "ERROR: cannot open log file or create log file.");
            abort();
        }
        if (dup2(log_fd, STDOUT_FILENO) == -1) {
            print_error_child(out, "ERROR: dup2 output redirected to log_fd failure.");
            abort();
        }
        free(log_file_path);
        close(log_fd);
        // redirect output of pipe to SYNC_FD
        if (dup2(newpipe[1], SYNC_FD) == -1) {
            print_error_child(out, "ERROR: dup2 output redirected to SYNC_FD failure.");
            abort();
        }
        close(newpipe[1]); // close the now redirected output pipe fd
        // update path
        char *og_path = getenv(PATH_ENV_VAR);
        char *daemons_dir = DAEMONS_DIR;
        if (og_path == NULL) {
            if (setenv(PATH_ENV_VAR, daemons_dir, 1) == -1) {
                print_error_child(out, "ERROR: cannot update PATH_ENV_VAR in child process.");
                abort();
            }
        } else {
            size_t new_path_dimension = strlen(og_path) + strlen(daemons_dir) + 2; // + 2 for : and 2;
            char *new_path = malloc(new_path_dimension);
            if (new_path == NULL) {
                print_error_child(out, "ERROR: failed malloc'd when attempt to prepend daemons_dir with PATH in child process.");
                abort();
            }
            snprintf(new_path, new_path_dimension, "%s:%s", daemons_dir, og_path);
            if (setenv(PATH_ENV_VAR, new_path, 1) == -1) {
                print_error_child(out, "ERROR: cannot update PATH_ENV_VAR in child process.");
                abort();
            }
            free(new_path);
        }
        // execute the execute
        if (execvpe(daemon->execute_name, daemon->execute_argv, __environ) == -1) {
            perror(strerror(errno));
            print_error_child(out, "ERROR: execvpe returned indicate a failure of process.");
            abort();
        }
    } else {
        // parent
        daemon->pid = pid;
        close(newpipe[1]); // close parent write pipe as parent is reading
        char mssg[1];
        recieved_alarm = false;
        alarm(CHILD_TIMEOUT);
        ssize_t checkByte = read(newpipe[0], mssg, 1);
        if (checkByte == 1) {
            sf_active(daemon->name, pid);
            daemon->status = status_active;
        } else {
            print_error_parent(out, "ERROR: could not synchronized with child Daemon.", current_status);
            sf_kill(daemon->name, pid);
            kill(pid, SIGKILL);
        }
        alarm(0);
        close(newpipe[0]); // close the read pipe as the child is either killed or successfully synchronized
        if (sigprocmask(SIG_SETMASK, prev_mask, NULL) < 0) {
            print_error_parent(out, "ERROR: couldn't unmask the mask sigchld after child has established.", current_status);
        }
    }
}

// -------------------------   END OF START CMD   ------------------------- //

// -------------------------  START OF STOP CMD   ------------------------- //

/**
 * @brief Stop the daemon given the pointer to the daemon. Rely on daemon_argv as well.
 *
 * @param daemon
 *      the pointer to the daemon we're stopping
 * @param current_status
 *      will determine behavior for outside of the this function to prevent crashing of program
 * @param out
 *      the output file stream to print our mssg.
 * @param stop_mask
 *      the mask we used to temporarily block out all SIGNALS beside SIGALRM and SIGCHLD
 */
void stop_daemon(sf_daemon *daemon, bool *current_status, FILE *out, sigset_t *stop_mask, sigset_t *child_mask, sigset_t *prev_mask) {
    if (daemon->status == status_exited || daemon->status == status_crashed) {
        daemon->status = status_inactive;
        sf_reset(daemon->name);
        return;
    } else if (daemon->status != status_active) {
        fprintf(out, "ERROR: Daemon '%s' is not active.\n", daemon_argv[0]);
        fflush(out);
        *current_status = false;
        return;
    }
    sf_stop(daemon->name, daemon->pid);
    daemon->status = status_stopping;
    int status = 0;
    if (sigprocmask(SIG_BLOCK, child_mask, prev_mask) < 0) {
        print_error_parent(out, "ERROR: couldn't install a mask sigchld when about to start child process.", current_status);
        return;
    }
    kill(daemon->pid, SIGTERM);
    recieved_alarm = false;
    alarm(CHILD_TIMEOUT);
    sigsuspend(stop_mask);
    if (recieved_alarm) {
    kill:
        kill(daemon->pid, SIGKILL);
        sf_kill(daemon->name, daemon->pid);
        fprintf(out, "ERROR: Daemon '%s' is required SIGKILL to terminate\n", daemon_argv[0]);
        fflush(out);
        *current_status = false;
    }
    pid_t test = waitpid(daemon->pid, &status, 0);
    alarm(0);
    if (test == -1) {
        goto kill;
    }
    if (sigprocmask(SIG_SETMASK, prev_mask, NULL) < 0) {
        print_error_parent(out, "ERROR: couldn't install a mask sigchld when about to start child process.", current_status);
        return;
    }
    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        sf_term(daemon->name, daemon->pid, exit_status);
        daemon->status = status_exited;
    } else if (WIFSIGNALED(status)) {
        int signal = WTERMSIG(status);
        sf_crash(daemon->name, daemon->pid, signal);
        daemon->status = status_crashed;
    }
    daemon->pid = 0;
}

int grim_reaper() {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        sf_daemon *daemon = find_daemon_id(pid);
        if (daemon == NULL) {
            return -1;
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            sf_term(daemon->name, daemon->pid, exit_status);
            daemon->status = status_exited;
        } else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            sf_crash(daemon->name, daemon->pid, signal);
            daemon->status = status_crashed;
        }
        daemon->pid = 0;
    }
    return 0;
}

// -------------------------   END OF STOP CMD    ------------------------- //

void run_cli(FILE *in, FILE *out) {
    sigset_t mask_all, mask_prev, mask_sigchld, mask_stop;
    if (sigfillset(&mask_all) < 0) {
        fprintf(stderr, "ERROR: couldn't create a mask all set.\n");
        exit(EXIT_FAILURE);
    }
    if (sigemptyset(&mask_sigchld) < 0 || sigaddset(&mask_sigchld, SIGCHLD) == -1) {
        fprintf(stderr, "ERROR: couldn't create a mask for SIGCHLD set.\n");
        exit(EXIT_FAILURE);
    }
    if (sigfillset(&mask_stop) < 0 || sigdelset(&mask_stop, SIGALRM) < 0 || sigdelset(&mask_stop, SIGCHLD) < 0) {
        fprintf(stderr, "ERROR: couldn't create a mask for SIGCHLD + SIGALRM set.\n");
        exit(EXIT_FAILURE);
    }
    if (sigprocmask(SIG_BLOCK, &mask_all, &mask_prev) < 0) {
        fprintf(stderr, "ERROR: couldn't set a mask all set when attemtpting to install Signals handler.\n");
        exit(EXIT_FAILURE);
    }
    struct sigaction handle_sig_int = { 0 };
    struct sigaction handle_sig_alrm = { 0 };
    struct sigaction handle_sig_chld = { 0 };
    handle_sig_int.sa_handler = handle_SIGINT;
    handle_sig_alrm.sa_handler = handle_SIGALRM;
    handle_sig_chld.sa_handler = hanle_SIGCHLD;
    if (sigaction(SIGINT, &handle_sig_int, NULL) < 0) {
        fprintf(stderr, "ERROR: could not install SIGINT handler.\n");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGALRM, &handle_sig_alrm, NULL) < 0) {
        fprintf(stderr, "ERROR: could not install SIGALRM handler.\n");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGCHLD, &handle_sig_chld, NULL) < 0) {
        fprintf(stderr, "ERROR: could not install SIGCHLD handler.\n");
        exit(EXIT_FAILURE);
    }
    if (sigprocmask(SIG_SETMASK, &mask_prev, NULL) < 0) {
        fprintf(stderr, "ERROR: couldn't unmask then mask all set when after install Signals handler.\n");
        exit(EXIT_FAILURE);
    }

    while (should_run) {
        sf_prompt();
        fprintf(out, "legion> ");
        fflush(out);
        if ((legion_argc = getline(&legion_argv, &legion_getline_buffer, in)) != -1) {
            bool current_status = true; // status of current executing command, 1 -> as expected, 0 -> something is wrong with command
            char *legion_argv_cursor = legion_argv;
            legion_argv_cursor[--legion_argc] = '\0'; // remove the last \n
            int length_of_command = 0; // for us to increment pass the command that is regonized by legion
            // read the type of command
            int type_of_command = read_type_of_command(legion_argv_cursor, &length_of_command);
            if (type_of_command == -2) { // malloc'd failure within
                print_error_parent(out, "ERROR: cannot malloc to read type of command.", &current_status);
            }
            if (!current_status) {
                print_sf_error(out, length_of_command, legion_argv);
                continue;
            }
            // parsing the rest of the argument after the current command
            legion_argv_cursor = legion_argv_cursor + length_of_command; // point at just after the command under normal execution.
            if (split_argument(&daemon_argv, legion_argv_cursor, &daemon_argc)) {
                print_error_parent(out, "ERROR: Failed to allocate memory to stored the arguments in a char**", &current_status);
            }
            if (!current_status) {
                print_sf_error(out, length_of_command, legion_argv);
                continue;
            }
            // behavior of determining if we get the correct args count for the respective command
            switch (type_of_command) {
            case REGISTER_COMMAND: {
                if (daemon_argc < 2) {
                    fprintf(out, "ERROR: Wrong number of args (given: %d, required at least %d) for command '%s'\n", daemon_argc, 2, legion_argv);
                    fflush(out);
                    current_status = false;
                }
                break;
            }
            case UNREGISTER_COMMAND:
            case STATUS_COMMAND:
            case START_COMMAND:
            case STOP_COMMAND:
            case LOGROTATE_COMMAND: {
                if (daemon_argc != 1) {
                    fprintf(out, "ERROR: Wrong number of args (given: %d, required: %d) for command '%s'\n", daemon_argc, 1, legion_argv);
                    fflush(out);
                    current_status = false;
                }
                break;
            }
            case STATUS_ALL_COMMAND:
            case QUIT_COMMAND:
            case HELP_COMMAND: {
                if (daemon_argc != 0) {
                    fprintf(out, "ERROR: Wrong number of args (given: %d, required: %d) for command '%s'\n", daemon_argc, 0, legion_argv);
                    fflush(out);
                    current_status = false;
                }
                break;
            }
            default:
                fprintf(out, "Unrecognized command: '%.*s'\n", (int)length_of_command, legion_argv);
                fflush(out);
                current_status = false;
                break;
            }
            if (!current_status) {
                sf_error("Error executing command");
                fprintf(out, "ERROR: Error Executing command: '%.*s'\n", (int)length_of_command, legion_argv);
                continue;
            }

            // at this point
            // command for 0 -> 8 contains the required item and be ready to execute
            // however the content within daemon_argv was not check and can may still contains error
            // daemon_argv still contains the malloc'd argument in a list
            // actual functionality of all rest of the function
            switch (type_of_command) {
            case HELP_COMMAND: {
                fprintf(out, "%s", HELP_MESSAGE);
                fflush(out);
                break;
            }
            case QUIT_COMMAND: {
                should_run = false;
                break;
            }
            case REGISTER_COMMAND: {
                char **daemon_argv_cp = copy_daemon_argv(daemon_argv, daemon_argc);
                if (daemon_argv_cp == NULL) {
                    print_error_parent(out, "ERROR: Allocation failure when attempting to create new daemon.", &current_status);
                    break;
                }

                sf_daemon *result = NULL;
                if ((result = find_daemon(daemon_argv_cp[0]))) {
                    fprintf(out, "ERROR: Daemon '%s' is already registered.\n", daemon_argv_cp[0]);
                    fflush(out);
                    current_status = false;
                    free_daemon_argv(&daemon_argv_cp, daemon_argc);
                    break;
                }

                sf_daemon *new_daemon = malloc(sizeof(sf_daemon));
                if (new_daemon == NULL) {
                    print_error_parent(out, "ERROR: Allocation failure when attempting to create new daemon.", &current_status);
                    break;
                }
                new_daemon->name = daemon_argv_cp[0];
                new_daemon->daemon_argc = daemon_argc;
                new_daemon->daemon_argv = daemon_argv_cp;
                new_daemon->execute_name = daemon_argv_cp[1];
                new_daemon->execute_argv = &(daemon_argv_cp[1]);
                new_daemon->status = status_inactive;
                new_daemon->pid = 0;
                new_daemon->log_version = 1;

                daemon_list.length++;
                new_daemon->links.next = daemon_list.first;
                daemon_list.first = new_daemon;
                sf_register(new_daemon->name, new_daemon->execute_name);
                break;
            }
            case UNREGISTER_COMMAND: {
                char *search_name = daemon_argv[0];
                sf_daemon *result = NULL;
                sf_daemon *prev = NULL;
                if ((result = find_daemon_2(search_name, &prev))) {
                    if (result->status != status_inactive) {
                        fprintf(out, "ERROR: Daemon '%s' is not inactive.\n", search_name);
                        fflush(out);
                        current_status = false;
                        break;
                    }
                    sf_unregister(search_name);
                    free_daemon_node(&result, &prev);
                }
                if (!result) {
                    fprintf(out, "ERROR: Daemon '%s' is not registered.\n", search_name);
                    fflush(out);
                    current_status = false;
                }
                break;
            }
            case STATUS_COMMAND: {
                sf_daemon *result = find_daemon(daemon_argv[0]);
                if (print_daemon_status(result, daemon_argv[0], out, &current_status)) {
                    print_error_parent(out, "ERROR: Allocation failure print the status of the daemon", &current_status);
                }
                break;
            }
            case STATUS_ALL_COMMAND: {
                if (print_all_daemon_status(out)) {
                    print_error_parent(out, "ERROR: Allocation failure running command status-all", &current_status);
                }
                break;
            }
            case START_COMMAND: {
                recieved_alarm = false; // given we're starting a command
                sf_daemon *current_daemon = find_daemon(daemon_argv[0]);
                if (!current_daemon) {
                    fprintf(out, "ERROR: Daemon '%s' is not registered.\n", daemon_argv[0]);
                    fflush(out);
                    current_status = false;
                    break;
                }
                struct stat dir_check;
                if (stat(LOGFILE_DIR, &dir_check) == -1 || !S_ISDIR(dir_check.st_mode)) {
                    if (mkdir(LOGFILE_DIR, 0777) == -1) {
                        fprintf(stderr, "ERROR: couldn't create a new directory for log files.\n");
                        current_status = false;
                        break;
                    }
                }
                start_daemon(current_daemon, &current_status, out, &mask_sigchld, &mask_prev);
                break;
            }
            case STOP_COMMAND: {
                recieved_alarm = false; // given we're stopping a command
                sf_daemon *current_daemon = find_daemon(daemon_argv[0]);
                if (!current_daemon) {
                    fprintf(out, "ERROR: Daemon '%s' is not registered.\n", daemon_argv[0]);
                    fflush(out);
                    current_status = false;
                    break;
                }
                stop_daemon(current_daemon, &current_status, out, &mask_stop, &mask_sigchld, &mask_prev);
                break;
            }
            case LOGROTATE_COMMAND: {
                sf_daemon *current_daemon = find_daemon(daemon_argv[0]);
                if (!current_daemon) {
                    fprintf(out, "ERROR: Daemon '%s' is not registered.\n", daemon_argv[0]);
                    fflush(out);
                    current_status = false;
                    break;
                }
                struct stat dir_check;
                if (stat(LOGFILE_DIR, &dir_check) == -1 || !S_ISDIR(dir_check.st_mode)) {
                    if (mkdir(LOGFILE_DIR, 0777) == -1) {
                        fprintf(stderr, "ERROR: couldn't create a new directory for log files.\n");
                        current_status = false;
                        break;
                    }
                }
                sf_logrotate(current_daemon->name);
                int current_log_version = current_daemon->log_version;
                for (int i = current_log_version; i > 0; i--) {
                    size_t old_log_file_dimension = snprintf(NULL, 0, "%s/%s.log.%d", LOGFILE_DIR, current_daemon->name, i - 1) + 1;
                    char *old_log_file_path = malloc(old_log_file_dimension);
                    size_t new_log_file_dimension = snprintf(NULL, 0, "%s/%s.log.%d", LOGFILE_DIR, current_daemon->name, i) + 1;
                    char *new_log_file_path = malloc(new_log_file_dimension);
                    if (old_log_file_path == NULL || new_log_file_path == NULL) {
                        print_error_parent(out, "ERROR: failed malloc'd when attempt to create the file path toward log_file", &current_status);
                        free(old_log_file_path);
                        free(new_log_file_path);
                        goto loop_end;
                    }
                    snprintf(old_log_file_path, old_log_file_dimension, "%s/%s.log.%d", LOGFILE_DIR, current_daemon->name, i - 1);
                    snprintf(new_log_file_path, new_log_file_dimension, "%s/%s.log.%d", LOGFILE_DIR, current_daemon->name, i);
                    if (access(old_log_file_path, F_OK) != 0) {
                        int old_log_fd;
                        if ((old_log_fd = open(old_log_file_path, O_WRONLY | O_CREAT, 0777)) == -1) {
                            print_error_parent(out, "ERROR: cannot open log file or create log file.", &current_status);
                            free(old_log_file_path);
                            free(new_log_file_path);
                            goto loop_end;
                        }
                        close(old_log_fd);
                    }
                    if (rename(old_log_file_path, new_log_file_path) != 0) {
                        print_error_parent(out, "ERROR: failed renaming log_file", &current_status);
                        free(old_log_file_path);
                        free(new_log_file_path);
                        goto loop_end;
                    }
                    free(old_log_file_path);
                    snprintf(new_log_file_path, new_log_file_dimension, "%s/%s.log.%d", LOGFILE_DIR, current_daemon->name, 0);
                    int new_log_fd;
                    if ((new_log_fd = open(new_log_file_path, O_WRONLY | O_CREAT, 0777)) == -1) {
                        print_error_parent(out, "ERROR: cannot open log file or create log file.", &current_status);
                        free(new_log_file_path);
                        goto loop_end;
                    }
                    close(new_log_fd);
                    free(new_log_file_path);
                }
            loop_end:
                if (!current_status) {
                    break;
                }
                current_log_version = ++(current_daemon->log_version);
                if (current_log_version == LOG_VERSIONS) {
                    size_t log_file_dimension = snprintf(NULL, 0, "%s/%s.log.%d", LOGFILE_DIR, current_daemon->name, current_daemon->log_version) + 1;
                    char *log_file_path = malloc(log_file_dimension);
                    if (log_file_path == NULL) {
                        print_error_parent(out, "ERROR: failed malloc'd when attempt to create the file path toward log_file.", &current_status);
                        break;
                    }
                    snprintf(log_file_path, log_file_dimension, "%s/%s.log.%d", LOGFILE_DIR, current_daemon->name, current_daemon->log_version);
                    if (access(log_file_path, F_OK) == 0 && unlink(log_file_path) != 0) {
                        print_error_parent(out, "ERROR: Failed to access and unlink the specific log file", &current_status);
                        free(log_file_path);
                        break;
                    }
                    free(log_file_path);
                    current_log_version = --(current_daemon->log_version);
                }
                if (current_daemon->status == status_active) {
                    recieved_alarm = false;
                    stop_daemon(current_daemon, &current_status, out, &mask_stop, &mask_sigchld, &mask_prev);
                    if (!current_status) {
                        break;
                    }
                    current_daemon->status = status_inactive;
                    recieved_alarm = false;
                    start_daemon(current_daemon, &current_status, out, &mask_sigchld, &mask_prev);
                    if (!current_status) {
                        break;
                    }
                }
                break;
            }
            default:
                break;
            }
            if (!current_status) {
                sf_error("Error executing command");
                fprintf(out, "ERROR: Error Executing command: '%.*s'\n", (int)length_of_command, legion_argv);
            }
            if (grim_reaper()) {
                fprintf(out, "ERROR: Cannot find daemon by id (SHOULD NEVER HAPPEN!!!)\n");
                fflush(out);
            }
        } else { // getline else
            int getline_errno = errno;
            if (grim_reaper()) {
                fprintf(out, "ERROR: Cannot find daemon by id (SHOULD NEVER HAPPEN!!!)\n");
                fflush(out);
            }
            int is_eof = feof(in);
            if (!is_eof && getline_errno == EINTR) {
                clearerr(in);
            } else {
                sf_error("Error reading command");
                fprintf(out, "ERROR: Error reading command, terminating program immediately.\n");
                fflush(out);
                should_run = false;
            }
        }
    }
    free_dynamic(&mask_stop, &mask_sigchld, &mask_prev);
}
