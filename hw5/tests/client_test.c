#include <criterion/criterion.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "debug.h"
#include "client_registry.h"
#include "user_registry.h"
#include "client.h"
#include "globals.h"
#include "excludes.h"

/* Maximum number of iterations performed for some tests. */
#define NITER (1000000)

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (10)

#define TEST_OUTPUT_DIR "test_output/"
#define TEST_REF_DIR "tests/rsrc/"

// A global user registry needs to be known in order to log in users.
USER_REGISTRY *user_registry;

// A global client registry is also needed in order to do things like check
// whether a user is already logged in.
CLIENT_REGISTRY *client_registry;

static void init(void) {
    mkdir(TEST_OUTPUT_DIR, 0777);
    user_registry = ureg_init();
    client_registry = creg_init();
}

static void fini(void) {
}

Test(client_suite, create, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    int ret = client_get_fd(client);
    cr_assert_eq(client_get_fd(client), ret,
		 "Returned file descriptor (%d) did not match expected (%d)",
		 ret, fd);
}

Test(client_suite, create_unref, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    client_unref(client, NULL);
}

Test(client_suite, create_login, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd = 42;
    char *name = "Alice";
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    int err = client_login(client, name);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
}

Test(client_suite, create_login_logout, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd = 42;
    char *name = "Alice";
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    int err = client_login(client, name);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    err = client_logout(client);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
}

Test(client_suite, register_login_twice_same, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    char *name = "Alice";
    int fd = 42;
    // For the multiple login check to work, the clients have to be obtained
    // by registering them with the client registry, not creating them directly.
    CLIENT *client1 = creg_register(client_registry, fd);
    cr_assert_not_null(client1, "Returned value was NULL");
    CLIENT *client2 = creg_register(client_registry, fd+1);
    cr_assert_not_null(client2, "Returned value was NULL");
    int err = client_login(client1, name);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    err = client_login(client2, name);
    cr_assert(err == -1, "Returned value (%d) was not -1", err);
}

Test(client_suite, register_login_twice_diff, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    char *name1 = "Alice";
    char *name2 = "Bob";
    int fd = 42;
    // For the multiple login check to work, the clients have to be obtained
    // by registering them with the client registry, not creating them directly.
    CLIENT *client1 = creg_register(client_registry, fd);
    cr_assert_not_null(client1, "Returned value was NULL");
    CLIENT *client2 = creg_register(client_registry, fd+1);
    cr_assert_not_null(client2, "Returned value was NULL");
    int err = client_login(client1, name1);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    err = client_login(client2, name2);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
}

Test(client_suite, register_login_many, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_USER
    cr_assert_fail("User module was not implemented");
#endif
    int NUSER = 50;
    int fd = 10;
    for(int i = 0; i < NUSER; i++) {
	char handle[10];
	sprintf(handle, "U%d", i);
	CLIENT *client = creg_register(client_registry, fd+i);
	cr_assert_not_null(client, "Returned value was NULL");
	int err = client_login(client, handle);
	cr_assert(err == 0, "Returned value (%d) was not 0", err);
	USER *u = client_get_user(client, 0);
	char *h = user_get_handle(u);
	cr_assert(!strcmp(h, handle), "Returned handle (%s) did not match expected (%s)",
		  h, handle);
    }
}

Test(client_suite, create_get_user, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    USER *user = client_get_user(client, 0);
    cr_assert_null(user, "Returned user pointer was not NULL");
}

Test(client_suite, create_get_mailbox, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    MAILBOX *mb = client_get_mailbox(client, 0);
    cr_assert_null(mb, "Returned mailbox pointer was not NULL");
}

Test(client_suite, create_login_get_user, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    char *name = "Alice";
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    int err = client_login(client, name);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    USER *user = client_get_user(client, 0);
    cr_assert_not_null(user, "Returned user pointer was NULL");
}

Test(client_suite, create_login_get_mailbox, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    char *name = "Alice";
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    int err = client_login(client, name);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    MAILBOX *mb = client_get_mailbox(client, 0);
    cr_assert_not_null(mb, "Returned mailbox pointer was NULL");
}

Test(client_suite, create_login_logout_get_user, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    char *name = "Alice";
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    int err = client_login(client, name);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    err = client_logout(client);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    USER *user = client_get_user(client, 0);
    cr_assert_null(user, "Returned user pointer was not NULL");
}

Test(client_suite, create_login_logout_get_mailbox, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    char *name = "Alice";
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    int err = client_login(client, name);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    err = client_logout(client);
    cr_assert(err == 0, "Returned value (%d) was not 0", err);
    MAILBOX *mb = client_get_mailbox(client, 0);
    cr_assert_null(mb, "Returned mailbox pointer was not NULL");
}

// Test sending packets.
// Here we have to create a legitimate file descriptor that points at a file where
// we can save the packet that is sent.

Test(client_suite, create_send_packet, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd;
    fd = open(TEST_OUTPUT_DIR"client_send_packet", O_CREAT|O_TRUNC|O_RDWR, 0644);
    cr_assert(fd > 0, "Failed to create output file");

    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");

    CHLA_PACKET_HEADER pkt = {0};
    char *payloadp = "This is a test payload";
    pkt.type = CHLA_ACK_PKT;
    pkt.payload_length = htonl(strlen(payloadp));
    pkt.msgid = htonl(0xdeadbeef);
    pkt.timestamp_sec = htonl(0x11223344);
    pkt.timestamp_nsec = htonl(0x55667788);

    int ret = client_send_packet(client, &pkt, payloadp);
    cr_assert_eq(ret, 0, "Returned value %d was not 0", ret);
    close(fd);

    ret = system("cmp "TEST_OUTPUT_DIR"client_send_packet "TEST_REF_DIR"client_send_packet");
    cr_assert_eq(ret, 0, "Packet sent did not match expected");
}

Test(client_suite, create_send_ack, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd;
    fd = open(TEST_OUTPUT_DIR"client_send_ack", O_CREAT|O_TRUNC|O_RDWR, 0644);
    cr_assert(fd > 0, "Failed to create output file");

    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");

    int ret = client_send_ack(client, 0xdeadbeef, NULL, 0);
    cr_assert_eq(ret, 0, "Returned value %d was not 0", ret);
    close(fd);

    // We can't just cmp the output with a reference, because the timestammps will
    // be different, in general.
    fd = open(TEST_OUTPUT_DIR"client_send_ack", O_RDONLY);
    cr_assert(fd > 0, "Failed to open output file");

    CHLA_PACKET_HEADER hdr;
    void *payloadp;
    int err = proto_recv_packet(fd, &hdr, &payloadp);
    cr_assert(err == 0, "Error reading back packet");

    int ok = (hdr.type == CHLA_ACK_PKT && hdr.msgid == 0xdeadbeef && hdr.payload_length == 0);
    cr_assert_eq(ok, 0, "Packet sent did not match expected");
}

Test(client_suite, create_send_nack, .init = init, .fini = fini, .timeout = 5) {
#ifdef NO_CLIENT
    cr_assert_fail("Client module was not implemented");
#endif
    int fd;
    fd = open(TEST_OUTPUT_DIR"client_send_nack", O_CREAT|O_TRUNC|O_RDWR, 0644);
    cr_assert(fd > 0, "Failed to create output file");

    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");

    int ret = client_send_nack(client, 0xdeadbeef);
    cr_assert_eq(ret, 0, "Returned value %d was not 0", ret);
    close(fd);

    // We can't just cmp the output with a reference, because the timestamps will
    // be different, in general.
    fd = open(TEST_OUTPUT_DIR"client_send_ack", O_RDONLY);
    cr_assert(fd > 0, "Failed to open output file");

    CHLA_PACKET_HEADER hdr;
    void *payloadp;
    int err = proto_recv_packet(fd, &hdr, &payloadp);
    cr_assert(err == 0, "Error reading back packet");

    int ok = (hdr.type == CHLA_NACK_PKT && hdr.msgid == 0xdeadbeef && hdr.payload_length == 0);
    cr_assert_eq(ok, 0, "Packet sent did not match expected");
}

/*
 * Concurrency test: Create a client, then create a number of threads to increment and
 * decrement its reference count, while holding one reference in reserve so that the
 * client will not be freed. Once the threads have finished, release the final reference
 * and make sure there has been no crash.
 */

static void *ref_thread(void *arg) {
    CLIENT *client = arg;
    unsigned int seed = 1;
    int balance = 0;
    for(int i = 0; i < NITER; i++) {
	int dir = 2 * (rand_r(&seed) % 2) - 1;
	if(balance + dir < 0)
	    dir = -dir;
	if(dir > 0)
	    client_ref(client, NULL);
	else
	    client_unref(client, NULL);
	balance += dir;
    }
    // Drain off any excess balance.
    while(balance--)
	client_unref(client, NULL);
    return NULL;
}

// Disable for the moment, because it produces a lot of debug output.
Test(client_suite, concurrent_ref, .timeout = 15) {
#ifdef NO_USER
    cr_assert_fail("User module was not implemented");
#endif
    int fd = 42;
    CLIENT *client = client_create(client_registry, fd);
    cr_assert_not_null(client, "Returned value was NULL");
    // Now holding one reference.

    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++)
	pthread_create(&tid[i], NULL, ref_thread, client);

    // Wait for all threads to finish.
    for(int i = 0; i < NTHREAD; i++)
	pthread_join(tid[i], NULL);

    // Release the final reference.
    client_unref(client, "which should be the last reference");
}

// Should check concurrent send.
// Should probably check concurrent login/logout, with login conflicts, if possible.

