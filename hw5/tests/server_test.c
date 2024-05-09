#include <criterion/criterion.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "debug.h"
#include "protocol.h"
#include "server.h"
#include "client_registry.h"
#include "user_registry.h"
#include "globals.h"
#include "excludes.h"

/* Name of the directory in which to put sockets. */
#define SOCKET_DIR "sockets"

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (15)

static char *chla_packet_type_names[] = {
    "NONE", "LOGIN", "LOGOUT", "USERS", "SEND",
    "ACK", "NACK", "MESG", "RCVD", "BOUNCE"
};

static int last_msgid = 0;
static pthread_mutex_t last_msgid_mutex = PTHREAD_MUTEX_INITIALIZER;

static int get_msgid() {
    pthread_mutex_lock(&last_msgid_mutex);
    int msgid = ++last_msgid;
    pthread_mutex_unlock(&last_msgid_mutex);
    return msgid;
}

static void init() {
    client_registry = creg_init();
    user_registry = ureg_init();

    // Sending packets to disconnected clients will cause termination by SIGPIPE
    // unless we take steps to ignore it.
    struct sigaction sact;
    sact.sa_handler = SIG_IGN;
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sigaction(SIGPIPE, &sact, NULL);
}

static void proto_init_packet(CHLA_PACKET_HEADER *pkt, CHLA_PACKET_TYPE type, size_t size) {
    memset(pkt, 0, sizeof(*pkt));
    pkt->type = type;
    pkt->msgid = htonl(get_msgid());
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
	perror("clock_gettime");
    }
    pkt->timestamp_sec = htonl(ts.tv_sec);
    pkt->timestamp_nsec = htonl(ts.tv_nsec);
    pkt->payload_length = htonl(size);
}

/*
 * Read a packet and check the header fields.
 * The packet and payload are returned.
 * If skip is nonzero, then asynchronous notifications not matching the
 * expected packet are ignored.
 */
static void check_packet(int fd, CHLA_PACKET_TYPE type, int id, int skip,
			 CHLA_PACKET_HEADER *pktp, void **payloadp) {
    void *data;
recv:
    int err = proto_recv_packet(fd, pktp, &data);
    if(payloadp)
        *payloadp = data;
    cr_assert_eq(err, 0, "Error reading back packet");
    if(skip && pktp->type != type &&
       (pktp->type == CHLA_ACK_PKT || pktp->type == CHLA_NACK_PKT ||
	pktp->type == CHLA_MESG_PKT || pktp->type == CHLA_RCVD_PKT ||
	pktp->type == CHLA_BOUNCE_PKT))
       goto recv;
    cr_assert_eq(pktp->type, type, "Packet type (%s) was not the expected type (%s)",
		 chla_packet_type_names[pktp->type], chla_packet_type_names[type]);
    if(id >= 0) {
	cr_assert_eq(ntohl(pktp->msgid), id, "ID in packet (%d) does not match expected (%d)",
		     ntohl(pktp->msgid), id);
    }
}

/*
 * For these tests, we will set up a connection betwen a test driver thread
 * and a server thread using a socket.  The driver thread will create and
 * bind the socket, then accept a connection.  The server thread will
 * connect and then hand off the file descriptor to the chla_client_service
 * function, as if the connection had been made over the network.
 * Communication over the connection will be done using whatever protocol
 * functions are linked, so if those don't work then the present tests will
 * likely also fail.
 */

/*
 * Thread function that connects to a socket with a specified name,
 * then hands off the resulting file descriptor to chla_client_service.
 * Errors cause the invoking test to fail.
 */
static void *server_thread(void *args) {
    char *name = args;  // socket name
    struct sockaddr_un sa;
    sa.sun_family = AF_LOCAL;
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s/%s", SOCKET_DIR, name);
    int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    cr_assert(sockfd >= 0, "Failed to create socket");
    int err = connect(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
    cr_assert(err == 0, "Failed to connect to socket");
    int *connfdp = malloc(sizeof(int));
    *connfdp = sockfd;
    chla_client_service(connfdp);
    return NULL;
}

/*
 * Set up a connection to a server thread, via a socket with a specified name.
 * The file descriptor to be used to communicate with the server is returned.
 * Errors cause the invoking test to fail.
 */
static int setup_connection(char *name) {
    // Set up socket to receive connection from server thread.
    int listen_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    cr_assert(listen_fd >= 0, "Failed to create socket");
    struct sockaddr_un sa;
    sa.sun_family = AF_LOCAL;
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s/%s", SOCKET_DIR, name);
    unlink((char *)sa.sun_path);
    int err = bind(listen_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
    cr_assert(err >= 0, "Failed to bind socket '%s'", sa.sun_path);
    err = listen(listen_fd, 0);
    cr_assert(err >= 0, "Failed to listen on socket");

    // Create server thread, passing the name of the socket.
    pthread_t tid;
    err = pthread_create(&tid, NULL, server_thread, name);
    cr_assert(err >= 0, "Failed to create server thread");

    // Accept connection from server thread.
    int connfd = accept(listen_fd, NULL, NULL);
    cr_assert(connfd >= 0, "Failed to accept connection");
    return connfd;
}    

/*
 * Perform a login operation on a specified connection, for a specified
 * user name.  Nothing is returned; errors cause the invoking test to fail.
 */
static void login_func(int connfd, char *uname, int ack) {
    CHLA_PACKET_HEADER pkt;
    proto_init_packet(&pkt, CHLA_LOGIN_PKT, strlen(uname));
    int msgid = ntohl(pkt.msgid);
    int err = proto_send_packet(connfd, &pkt, uname);
    cr_assert_eq(err, 0, "Send packet returned an error");
    memset(&pkt, 0, sizeof(pkt));
    check_packet(connfd, ack? CHLA_ACK_PKT : CHLA_NACK_PKT, msgid, 0, &pkt, NULL);
}

/*
 * Perform a logout operation on a specified connection.
 * Nothing is returned; errors cause the invoking test to fail.
 */
static void logout_func(int connfd, int ack) {
    CHLA_PACKET_HEADER pkt;
    proto_init_packet(&pkt, CHLA_LOGOUT_PKT, 0);
    int msgid = ntohl(pkt.msgid);
    int err = proto_send_packet(connfd, &pkt, NULL);
    cr_assert_eq(err, 0, "Send packet returned an error");
    memset(&pkt, 0, sizeof(pkt));
    check_packet(connfd, ack? CHLA_ACK_PKT : CHLA_NACK_PKT, msgid, 0, &pkt, NULL);
}

/*
 * Perform a "users" operation on a specified connection.
 * An ACK is expected and the payload is returned.
 */
static void users_func(int connfd, void **payloadp, size_t *len) {
    CHLA_PACKET_HEADER pkt;
    proto_init_packet(&pkt, CHLA_USERS_PKT, 0);
    int msgid = ntohl(pkt.msgid);
    int err = proto_send_packet(connfd, &pkt, NULL);
    cr_assert_eq(err, 0, "Send packet returned an error");
    memset(&pkt, 0, sizeof(pkt));
    check_packet(connfd, CHLA_ACK_PKT, msgid, 0, &pkt, payloadp);
    *len = ntohl(pkt.payload_length);
}

/*
 * Perform a "send" operation on a specified connection.
 * An ACK is expected, but intervening RCVD notifications are skipped.
 */
static void send_func(int connfd, char *handle, char *msg) {
    CHLA_PACKET_HEADER pkt;
    int len = strlen(handle) + 2 + strlen(msg);
    char *payload = malloc(len+1);
    snprintf(payload, len+1, "%s\r\n%s", handle, msg);
    proto_init_packet(&pkt, CHLA_SEND_PKT, len);
    int msgid = ntohl(pkt.msgid);
    int err = proto_send_packet(connfd, &pkt, payload);
    cr_assert_eq(err, 0, "Send packet returned an error");
    free(payload);
    memset(&pkt, 0, sizeof(pkt));
    check_packet(connfd, CHLA_ACK_PKT, msgid, 1, &pkt, NULL);
}

/*
 * Test driver thread that sends a packet other than LOGIN over the connection
 * and checks that NACK is received.
 */
static void *ping_thread(void *arg) {
    return NULL;
}

/*
 * Create a connection and then "ping" it to elicit a NACK.
 */
Test(server_suite, ping, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "ping.sock";
    int connfd = setup_connection(sockname);

    CHLA_PACKET_HEADER pkt;
    proto_init_packet(&pkt, CHLA_LOGOUT_PKT, 0);
    int msgid = ntohl(pkt.msgid);
    int err = proto_send_packet(connfd, &pkt, NULL);
    cr_assert_eq(err, 0, "Send packet returned an error");
    check_packet(connfd, CHLA_NACK_PKT, msgid, 0, &pkt, NULL);
    close(connfd);
}

/*
 * Create a connection, log in, then close the connection.
 */
Test(server_suite, valid_login, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "alice.sock";
    char *username = "Alice";
    int connfd = setup_connection(sockname);
    login_func(connfd, username, 1);
    close(connfd);
}

/*
 * I would test a LOGIN with no payload, except that my server
 * allocates an empty payload and goes ahead and does a login with
 * an empty username.  So probably not fair.
 */

/*
 * Create a connection, log in, then try to log in again under
 * a different user ID.
 */
Test(server_suite, login_again_same_user, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "alice.sock";
    char *username1 = "Alice";
    char *username2 = "Bob";
    int connfd = setup_connection(sockname);
    login_func(connfd, username1, 1);
    login_func(connfd, username2, 0);
    close(connfd);
}

/*
 * Create a connection, log in, then log out.
 */
Test(server_suite, login_logout, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "alice.sock";
    char *username = "Alice";
    int connfd = setup_connection(sockname);
    login_func(connfd, username, 1);
    logout_func(connfd, 1);
    close(connfd);
}

/*
 * Create a connection, log in, log out, then log in again as the same user.
 * (This test exposed a race in my server implementation.)
 */
Test(server_suite, login_logout_login, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "alice.sock";
    char *username = "Alice";
    int connfd = setup_connection(sockname);
    login_func(connfd, username, 1);
    logout_func(connfd, 1);
    login_func(connfd, username, 1);
    close(connfd);
}

/*
 * Create two different connections and log in as different users on each.
 */
Test(server_suite, login_two_conns_diff, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname1 = "alice.sock";
    char *username1 = "Alice";
    char *sockname2 = "bob.sock";
    char *username2 = "Bob";
    int connfd1 = setup_connection(sockname1);
    int connfd2 = setup_connection(sockname2);
    login_func(connfd1, username1, 1);
    login_func(connfd2, username2, 1);
    close(connfd1);
    close(connfd2);
}

/*
 * Create two different connections and try to log in as the same user on each.
 */
Test(server_suite, login_two_conns_same, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname1 = "alice.sock";
    char *username1 = "Alice";
    char *sockname2 = "bob.sock";
    int connfd1 = setup_connection(sockname1);
    int connfd2 = setup_connection(sockname2);
    login_func(connfd1, username1, 1);
    login_func(connfd2, username1, 0);
    close(connfd1);
    close(connfd2);
}

/*
 * Create two different connections and log in as different users on each.
 * Then issue a "users" command on one of the connections.
 */
Test(server_suite, login_two_conns_diff_users, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname1 = "alice.sock";
    char *username1 = "Alice";
    char *sockname2 = "bob.sock";
    char *username2 = "Bob";
    int connfd1 = setup_connection(sockname1);
    int connfd2 = setup_connection(sockname2);
    login_func(connfd1, username1, 1);
    login_func(connfd2, username2, 1);
    char *payloadp;
    size_t len = 0;
    size_t exp_len = strlen(username1) + 2 + strlen(username2) + 2;
    users_func(connfd1, (void **)&payloadp, &len);
    cr_assert_eq(len, exp_len, "Payload length (%ld) was not the expected value (%ld)",
		 len, exp_len);
    cr_assert(!strncmp(payloadp, "Alice\r\nBob\r\n", exp_len) || !strncmp(payloadp, "Bob\r\nAlice\r\n", exp_len));
    free(payloadp);
    close(connfd1);
    close(connfd2);
}

/*
 * Create two different connections and log in as different users on each.
 * Then send a message from one to the other.
 */
Test(server_suite, login_two_conns_diff_users_send1, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname1 = "alice.sock";
    char *username1 = "Alice";
    char *sockname2 = "bob.sock";
    char *username2 = "Bob";
    int connfd1 = setup_connection(sockname1);
    int connfd2 = setup_connection(sockname2);
    login_func(connfd1, username1, 1);
    login_func(connfd2, username2, 1);
    char *msg = "Hello!";
    send_func(connfd1, username2, msg);

    // Receive packet on connfd2 and check content.
    char *payloadp = NULL;
    CHLA_PACKET_HEADER hdr;
    int err = proto_recv_packet(connfd2, &hdr, (void **)&payloadp);
    cr_assert_eq(err, 0, "Error receiving packet");
    size_t len = ntohl(hdr.payload_length);
    size_t exp_len = strlen(username1) + 2 + strlen(msg);
    cr_assert_eq(len, exp_len, "Length of received packet (%ld) did not match expected (%d)",
		 len, exp_len);
    char exp_msg[exp_len + 1];
    snprintf(exp_msg, exp_len + 1, "%s\r\n%s", username1, msg);
    char rcvd_msg[len+1];
    strncpy(rcvd_msg, payloadp, len+1);
    cr_assert(!strncmp(rcvd_msg, exp_msg, exp_len),
	      "Received message (%s) did not match expected (%s)",
	      rcvd_msg, exp_msg);
    close(connfd1);
    close(connfd2);
}

/*
 * Create a connection, log in, send a message to ourself and check
 * that it is received (possibly with an intervening ACK).
 */
Test(server_suite, login_send_self_recv, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "alice.sock";
    char *username = "Alice";
    int connfd = setup_connection(sockname);
    login_func(connfd, username, 1);
    char *msg = "Hello!";
    send_func(connfd, username, msg);

    // Receive packet on connfd and check content.
recv:
    char *payloadp = NULL;
    CHLA_PACKET_HEADER hdr;
    int err = proto_recv_packet(connfd, &hdr, (void **)&payloadp);
    cr_assert_eq(err, 0, "Error receiving packet");
    if(hdr.type == CHLA_ACK_PKT)
	goto recv;
    CHLA_PACKET_TYPE exp_type = CHLA_MESG_PKT;
    cr_assert_eq(hdr.type, exp_type,
		 "Received packet type (%s) was not the expected type (%s)",
		 chla_packet_type_names[hdr.type], chla_packet_type_names[exp_type]);
    size_t len = ntohl(hdr.payload_length);
    size_t exp_len = strlen(username) + 2 + strlen(msg);
    cr_assert_eq(len, exp_len, "Length of received packet (%ld) did not match expected (%d)",
		 len, exp_len);
    char exp_msg[exp_len + 1];
    snprintf(exp_msg, exp_len + 1, "%s\r\n%s", username, msg);
    char rcvd_msg[len+1];
    strncpy(rcvd_msg, payloadp, len+1);
    cr_assert(!strncmp(rcvd_msg, exp_msg, exp_len),
	      "Received message (%s) did not match expected (%s)",
	      rcvd_msg, exp_msg);
    close(connfd);
}

/*
 * Concurrently create many connections and log in a different
 * user on each one.  Then send USERS and check the payload that
 * is returned.
 */

struct login_thread_args {
    char sockname[32];
    char username[32];
};

void *login_thread(void *args) {
    struct login_thread_args *ap = args;
    int connfd = setup_connection(ap->sockname);
    login_func(connfd, ap->username, 1);
    return (void *)(long)connfd;
}

Test(server_suite, login_many_users, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "login_many_users.sock";

    pthread_t tids[NTHREAD];
    // The first connection will be used later to issue USERS.
    int connfd = setup_connection(sockname);
    login_func(connfd, "u0", 1);

    // The rest of the connections are made concurrently.
    for(int i = 1; i < NTHREAD; i++) {
	struct login_thread_args *args = malloc(sizeof(struct login_thread_args));
	snprintf(args->sockname, sizeof(args->sockname), "%s.%d", sockname, i);
	snprintf(args->username, sizeof(args->username), "u%d", i);
	int err = pthread_create(&tids[i], NULL, login_thread, args);
	cr_assert(err >= 0, "Failed to create test thread");
    }
    // Wait for all the threads to finish.
    int fds[NTHREAD];
    for(int i = 1; i < NTHREAD; i++)
	pthread_join(tids[i], (void *)&fds[i]);

    // Send USERS over the first connection and get the response.
    char *payloadp;
    size_t len = 0;
    users_func(connfd, (void **)&payloadp, &len);
    char *str = calloc(len+1, 1);
    strncpy(str, payloadp, len);
    
    // Check the response
    //fprintf(stderr, "\n%s\n", str);
    FILE *f = fmemopen(str, strlen(str), "r");
    int nlines = 0;
    char *ln = NULL;
    size_t sz = 0;
    while(getline(&ln, &sz, f) > 0) {
	nlines++;
	int count = 0;
	for(int i = 0; i < NTHREAD; i++) {
	    char line[64];
	    snprintf(line, sizeof(line), "u%d\r\n", i);
	    if(!strcmp(ln, line))
		count++;
	}
	cr_assert_eq(count, 1, "USERS output was incorrect: \n%s\n", str);
	free(ln);
	sz = 0; ln = NULL;
    }
    free(ln);
    fclose(f);
    cr_assert_eq(nlines, NTHREAD, "Number of lines (%d) did not match expected (%d)",
		 nlines, NTHREAD);

    // Close all the connections.
    for(int i = 1; i < NTHREAD; i++)
	close(fds[i]);
    close(connfd);

    // Test will in general terminate before all threads have finished.
}

/*
 * Concurrently create many connections and log in a different
 * user on each one.  Each client sends a message to the first client,
 * who consumes and checks them.
 */

struct login_send_thread_args {
    char sockname[32];
    char username[32];
    char *to;
};

void *login_send_thread(void *args) {
    struct login_send_thread_args *ap = args;
    int connfd = setup_connection(ap->sockname);
    login_func(connfd, ap->username, 1);
    // Send my username to the main thread.
    send_func(connfd, ap->to, ap->username);
    return (void *)(long)connfd;
}

Test(server_suite, login_send_many_users, .init = init, .timeout = 5) {
#ifdef NO_SERVER
    cr_assert_fail("Server module was not implemented");
#endif
    char *sockname = "login_many_users.sock";
    char *main_thread_name = "u0";

    pthread_t tids[NTHREAD];
    // The first connection will be used later to receive messages.
    int connfd = setup_connection(sockname);
    login_func(connfd, main_thread_name, 1);

    // The rest of the connections are made concurrently.
    for(int i = 1; i < NTHREAD; i++) {
	struct login_send_thread_args *args = malloc(sizeof(struct login_send_thread_args));
	snprintf(args->sockname, sizeof(args->sockname), "%s.%d", sockname, i);
	snprintf(args->username, sizeof(args->username), "u%d", i);
	args->to = main_thread_name;
	int err = pthread_create(&tids[i], NULL, login_send_thread, args);
	cr_assert(err >= 0, "Failed to create test thread");
    }
    // Wait for all the threads to finish.
    int fds[NTHREAD];
    for(int i = 1; i < NTHREAD; i++)
	pthread_join(tids[i], (void *)&fds[i]);

    // Close all the other connections.
    for(int i = 1; i < NTHREAD; i++)
	close(fds[i]);

    // Consume messages that ought to have accumulated.
    for(int i = 1; i < NTHREAD; i++) {
	CHLA_PACKET_HEADER hdr;
	char *payloadp;
	int err = proto_recv_packet(connfd, &hdr, (void **)&payloadp);
	cr_assert_eq(err, 0, "Error receiving packet");
	size_t len = ntohl(hdr.payload_length);
	// I could check the content, but I am not going to bother.
	//printf("MESSAGE #%d, LENGTH %ld\n", i, len);
	(void)len;
    }
    close(connfd);

    // Shutdown all connections and terminate.
    creg_shutdown_all(client_registry);
}
