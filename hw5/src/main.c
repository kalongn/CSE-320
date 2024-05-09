#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include "csapp.h"

static volatile sig_atomic_t should_run = 1;

void handle_SIGHUP() {
    should_run = 0;
}
void handle_SIGPIPE() {
    // this is just to interrupt read and write such that they return -1.
}

static void terminate(int);

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */
int main(int argc, char *argv[]) {
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    sigset_t mask_all, mask_prev;
    if (sigfillset(&mask_all) < 0) {
        fprintf(stderr, "ERROR: couldn't create a mask all set.\n");
        exit(EXIT_SUCCESS);
    }
    if (sigprocmask(SIG_BLOCK, &mask_all, &mask_prev) < 0) {
        fprintf(stderr, "ERROR: couldn't set a mask all set when attemtpting to install Signals handler.\n");
        exit(EXIT_SUCCESS);
    }
    struct sigaction handle_sig_hup = { .sa_handler = handle_SIGHUP };
    if (sigaction(SIGHUP, &handle_sig_hup, NULL) < 0) {
        fprintf(stderr, "ERROR: could not install SIGHUP handler.\n");
        exit(EXIT_SUCCESS);
    }
    struct sigaction handle_sigpipe = { .sa_handler = handle_SIGPIPE };
    if (sigaction(SIGPIPE, &handle_sigpipe, NULL) < 0) {
        fprintf(stderr, "ERROR: could not install SIGHUP handler.\n");
        exit(EXIT_SUCCESS);
    }
    if (sigprocmask(SIG_SETMASK, &mask_prev, NULL) < 0) {
        fprintf(stderr, "ERROR: couldn't unmask then mask all set when after install Signals handler.\n");
        exit(EXIT_SUCCESS);
    }

    int port = -1;

    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            break;
        }
    }
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "Usage: -p <port [1024, 65535]> \n");
        exit(EXIT_SUCCESS);
    }

    // Perform required initializations of the client_registry and
    // player_registry.
    user_registry = ureg_init();
    client_registry = creg_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    char *string_port;
    size_t length = snprintf(NULL, 0, "%d", port) + 1;
    if ((string_port = malloc(length)) == NULL) {
        fprintf(stderr, "ERROR: cannot malloc the port number to open a socket.\n");
        exit(EXIT_SUCCESS);
    }
    snprintf(string_port, length, "%d", port);


    int listenfd, *connfdp = NULL;
    socklen_t clientlen;
    struct sockaddr_storage cliendtadrr;
    pthread_t tid;

    listenfd = open_listenfd(string_port);
    if (listenfd < 0) {
        free(string_port);
        fprintf(stderr, "ERROR: cannot open socket.\n");
        exit(EXIT_SUCCESS);
    }
    while (should_run) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));
        if (connfdp == NULL) {
            fprintf(stderr, "ERROR: cannot malloc for a new connection address in charla server\n");
            continue;
        }
        if ((*connfdp = accept(listenfd, (struct sockaddr *)&cliendtadrr, &clientlen)) < 0) {
            fprintf(stderr, "ERROR: cannot accept new connection in charala Server\n");
            free(connfdp);
            continue;
        }
        int ret_status = 0;
        if ((ret_status = pthread_create(&tid, NULL, chla_client_service, connfdp)) < 0) {
            fprintf(stderr, "ERROR: cannot create new thread to accept new connection in charala Server, %s\n", strerror(errno));
            free(connfdp);
            continue;
        }
    }
    if (close(listenfd) < 0) {
        fprintf(stderr, "ERROR: failed to close the listening socket.\n");
    }
    free(string_port);
    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}
