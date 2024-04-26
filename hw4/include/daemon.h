#include <stdlib.h>

typedef struct sf_daemon {
    char *name;                 // pointer to the name of the daemon (dynamic)
    volatile pid_t pid;                    // pid of the daemon
    volatile int status;                 // current status of the daemon
    int log_version;            // the maximum log_version
    int daemon_argc;            // used for free the entire 2D array
    char **daemon_argv;         // used for free 
    char *execute_name;         // name of executable for the daemon (dynamic)
    char **execute_argv;        // name of executable argv for the daemon (dynamic)
    struct {
        struct sf_daemon *next;       // pointer to next daemon
    } links;
} sf_daemon;

struct {
    volatile int length;
    struct sf_daemon *first;
} daemon_list = { 0, NULL };


char *daemon_state_names[] = {
    [status_unknown] = "unknown",
    [status_inactive] = "inactive",
    [status_starting] = "starting",
    [status_active] = "active",
    [status_stopping] = "stopping",
    [status_exited] = "exited",
    [status_crashed] = "crashed"
};
