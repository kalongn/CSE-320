#include <criterion/criterion.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "debug.h"
#include "client_registry.h"
#include "user.h"
#include "excludes.h"

/* Maximum number of iterations performed for some tests. */
#define NITER (1000000)

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (10)

/* Number of messages we send in concurrency stress test. */
#define NMESSAGE (10000)

Test(mailbox_suite, init, .timeout = 5) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *name = "Alice";
    MAILBOX *mb = mb_init(name);
    cr_assert_not_null(mb, "Returned value was NULL");

    char *un = mb_get_handle(mb);
    cr_assert(!strcmp(un, name), "Mailbox handle (%s) does not match expected (%s)",
	      un, name);
}

Test(mailbox_suite, init_unref, .timeout = 5) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *name = "Alice";
    MAILBOX *mb = mb_init(name);
    cr_assert_not_null(mb, "Returned value was NULL");

    mb_unref(mb, NULL);
}

Test(mailbox_suite, init_shutdown_unref, .timeout = 5) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *name = "Alice";
    MAILBOX *mb = mb_init(name);
    cr_assert_not_null(mb, "Returned value was NULL");

    mb_shutdown(mb);
    mb_unref(mb, NULL);
}

/*
 * We can do basic tests of mb_next_entry() by first adding entries to the mailbox
 * and then reading out the entries we added.
 */

Test(mailbox_suite, init_add_msg_next, .timeout = 5) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *alice = "Alice";
    MAILBOX *from = mb_init(alice);
    cr_assert_not_null(from, "Returned value was NULL");

    char *bob = "Bob";
    MAILBOX *to = mb_init(bob);
    cr_assert_not_null(to, "Returned value was NULL");

    char *body = "This is a message!";
    int len = strlen(body)+1;
    int msgid = 12345;
    mb_add_message(to, msgid, from, body, len);

    MAILBOX_ENTRY *entry = mb_next_entry(to);
    cr_assert_not_null(entry, "Return value was NULL");

    cr_assert_eq(entry->type, MESSAGE_ENTRY_TYPE,
		 "Entry type (%d) did not match expected (%d)",
		 entry->type, MESSAGE_ENTRY_TYPE);
    MESSAGE *msg = &entry->content.message;
    cr_assert_eq(msg->msgid, msgid, "Message ID (%d) did not match expected (%d)",
		 msg->msgid, msgid);
    cr_assert_eq(msg->from, from, "Message 'from' (%p) did not match expected (%p)",
		 msg->from, from);
    cr_assert_eq(msg->length, len, "Message length (%d) did not match expected (%d)",
		 msg->length, len);
    cr_assert(!strcmp(msg->body, body), "Message body (%s) did not match expected (%s)",
	      msg->body, body);
}

Test(mailbox_suite, init_add_notice_next, .timeout = 5) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *alice = "Alice";
    MAILBOX *from = mb_init(alice);
    cr_assert_not_null(from, "Returned value was NULL");

    char *bob = "Bob";
    MAILBOX *to = mb_init(bob);
    cr_assert_not_null(to, "Returned value was NULL");

    int msgid = 12345;
    NOTICE_TYPE type = BOUNCE_NOTICE_TYPE;
    mb_add_notice(to, type, msgid);

    MAILBOX_ENTRY *entry = mb_next_entry(to);
    cr_assert_not_null(entry, "Return value was NULL");

    cr_assert_eq(entry->type, NOTICE_ENTRY_TYPE,
		 "Entry type (%d) did not match expected (%d)",
		 entry->type, NOTICE_ENTRY_TYPE);
    NOTICE *ntc = &entry->content.notice;
    cr_assert_eq(ntc->type, type, "Notice type (%d) did not match expected (%d)",
		 ntc->type, type);
    cr_assert_eq(ntc->msgid, msgid, "Message ID (%d) did not match expected (%d)",
		 ntc->msgid, msgid);
}

// Test that mb_next_entry() on an empty mailbox blocks.

Test(mailbox_suite, next_entry_empty, .timeout = 5, .signal = SIGALRM) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *alice = "Alice";
    MAILBOX *mb = mb_init(alice);
    cr_assert_not_null(mb, "Returned value was NULL");

    // Generate a signal after a short delay.  Receipt of the signal
    // is the expected result.  Return from mb_next_entry() is not.
    alarm(1);
    MAILBOX_ENTRY *entry = mb_next_entry(mb);
    (void)entry;
    cr_assert(0, "mb_next_entry() returned when it should not have");
}

// Test that mb_shutdown() on an empty mailbox unblocks a thread in mb_next_entry().

void *mb_shutdown_thread(void *arg) {
    sleep(1);
    mb_shutdown((MAILBOX *)arg);
    return NULL;
}

Test(mailbox_suite, next_entry_shutdown, .timeout = 5) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *alice = "Alice";
    MAILBOX *mb = mb_init(alice);
    cr_assert_not_null(mb, "Returned value was NULL");

    // Create a thread to call mb_shutdown after a short delay.
    pthread_t tid;
    pthread_create(&tid, NULL, mb_shutdown_thread, mb);

    // Here the expected result is NULL return from mb_next_entry()
    // before the test times out.
    MAILBOX_ENTRY *entry = mb_next_entry(mb);
    cr_assert_null(entry, "A non-NULL entry was returned from an empty mailbox");
}

// Test the functioning of the discard hook.

static volatile int msgid_to_be_discarded;
static volatile sig_atomic_t msg_discarded = 0;

void discard_func(MAILBOX_ENTRY *entry) {
    cr_assert_not_null(entry, "Discard function called with NULL argument");
    cr_assert_eq(entry->content.message.msgid, msgid_to_be_discarded,
		 "The ID of the message being discarded (%d) does not match expected (%d)",
		 entry->content.message.msgid, msgid_to_be_discarded);
    msg_discarded = 1;
}

Test(mailbox_suite, init_add_msg_unref, .timeout = 5) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *alice = "Alice";
    MAILBOX *from = mb_init(alice);
    cr_assert_not_null(from, "Returned value was NULL");

    char *bob = "Bob";
    MAILBOX *to = mb_init(bob);
    cr_assert_not_null(to, "Returned value was NULL");

    mb_set_discard_hook(to, discard_func);

    char *body = "This is a message!";
    int len = strlen(body)+1;
    int msgid = 12345;
    mb_add_message(to, msgid, from, strdup(body), len);

    // Unref the recipient mailbox.  This should cause it to be finalized,
    // and the discard hook should be called on the undelivered message.
    msgid_to_be_discarded = msgid;
    mb_unref(to, NULL);
    while(!msg_discarded)
	sleep(1);
}

// Concurrency stress test: Multiple threads adding messages,
// one thread consuming and checking them.

struct producer_args {
    int thread_id;
    MAILBOX *mailbox;
};

static void *producer_thread(void *arg) {
    struct producer_args *ap = arg;
    int tid = ap->thread_id;
    MAILBOX *to = ap->mailbox;
    MAILBOX *from = mb_init("Anonymous");
    char *body = "This is a message!";
    int len = strlen(body)+1;
    for(int id = 1; id <= NMESSAGE; id++)
	mb_add_message(to, (tid << 16) | id, from, strdup(body), len);
    return NULL;
}

static void *consumer_thread(void *arg) {
    MAILBOX *mb = arg;
    MAILBOX_ENTRY *entry;
    // Last message ID observed from each thread.
    int last_id[NTHREAD];
    while((entry = mb_next_entry(mb)) != NULL) {
	int from = entry->content.message.msgid >> 16;
	int id = entry->content.message.msgid & 0xffff;
	cr_assert(0 <= from && from < NTHREAD, "ID in message (%d) was invalid", id);
	last_id[from]++;
	cr_assert_eq(id, last_id[from],
		     "ID in message (%d) was not in sequence (expected %d)",
		     id, last_id[from]);
	free(entry->content.message.body);
    }
    return NULL;
}

Test(mailbox_suite, concurrent_add_consume, .timeout = 15) {
#ifdef NO_MAILBOX
    cr_assert_fail("User module was not implemented");
#endif
    char *name = "Alice";
    MAILBOX *mb = mb_init(name);
    cr_assert_not_null(mb, "Returned value was NULL");
    // Now holding one reference.

    // Create consumer thread.
    pthread_t cid;
    pthread_create(&cid, NULL, consumer_thread, mb);

    // Create producer threads.
    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++) {
	struct producer_args *args = malloc(sizeof(*args));
	args->thread_id = i;
	args->mailbox = mb;
	pthread_create(&tid[i], NULL, producer_thread, args);
    }

    // Wait for all producers to finish.
    for(int i = 0; i < NTHREAD; i++)
	pthread_join(tid[i], NULL);

    // Shut down mailbox and wait for consumer to finish.
    mb_shutdown(mb);
    pthread_join(cid, NULL);
}

/*
 * Concurrency test: Create a mailbox, then create a number of threads to increment and
 * decrement its reference count, while holding one reference in reserve so that the
 * user will not be freed. Once the threads have finished, release the final reference
 * and make sure there has been no crash.
 */

static void *ref_thread(void *arg) {
    MAILBOX *mb = arg;
    unsigned int seed = 1;
    int balance = 0;
    for(int i = 0; i < NITER; i++) {
	int dir = 2 * (rand_r(&seed) % 2) - 1;
	if(balance + dir < 0)
	    dir = -dir;
	if(dir > 0)
	    mb_ref(mb, NULL);
	else
	    mb_unref(mb, NULL);
	balance += dir;
    }
    // Drain off any excess balance.
    while(balance--)
	mb_unref(mb, NULL);
    return NULL;
}

Test(mailbox_suite, concurrent_ref, .timeout = 15) {
#ifdef NO_MAILBOX
    cr_assert_fail("Mailbox module was not implemented");
#endif
    char *name = "Alice";
    MAILBOX *mb = mb_init(name);
    cr_assert_not_null(mb, "Returned value was NULL");
    // Now holding one reference.

    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++)
	pthread_create(&tid[i], NULL, ref_thread, mb);

    // Wait for all threads to finish.
    for(int i = 0; i < NTHREAD; i++)
	pthread_join(tid[i], NULL);

    // Release the final reference.
    mb_unref(mb, "which should be the last reference");
}
