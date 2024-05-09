#include <criterion/criterion.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "client_registry.h"
#include "user.h"
#include "excludes.h"

/* Maximum number of iterations performed for some tests. */
#define NITER (1000000)

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (10)

/* Number of users we create. */
#define NUSER (100)

Test(user_suite, create, .timeout = 5) {
#ifdef NO_USER
    cr_assert_fail("User module was not implemented");
#endif
    char *name = "Alice";
    USER *user = user_create(name);
    cr_assert_not_null(user, "Returned value was NULL");

    char *un = user_get_handle(user);
    cr_assert(!strcmp(un, name), "User name (%s) does not match expected (%s)",
	      un, name);
}

Test(user_suite, create_modify_handle, .timeout = 5) {
#ifdef NO_USER
    cr_assert_fail("User module was not implemented");
#endif
    char *name = "Alice";
    char *temp = strdup(name);
    USER *user = user_create(temp);
    cr_assert_not_null(user, "Returned value was NULL");
    *temp = '\0';
    free(temp);

    char *un = user_get_handle(user);
    cr_assert(!strcmp(un, name), "User name (%s) does not match expected (%s)",
	      un, name);
}

Test(user_suite, create_multiple, .timeout = 5) {
#ifdef NO_USER
    cr_assert_fail("User module was not implemented");
#endif
    char *Alice = "Alice";
    char *Bob = "Bob";
    char *Carol = "Carol";

    USER *alice = user_create(Alice);
    USER *bob = user_create(Bob);
    USER *carol = user_create(Carol);

    char *un = user_get_handle(bob);
    cr_assert(!strcmp(un, Bob), "User name (%s) does not match expected (%s)",
	      un, Bob);
    user_unref(bob, "for pointer being discarded");
    un = user_get_handle(alice);
    cr_assert(!strcmp(un, Alice), "User name (%s) does not match expected (%s)",
	      un, Alice);
    user_unref(alice, "for pointer being discarded");
    un = user_get_handle(carol);
    cr_assert(!strcmp(un, Carol), "User name (%s) does not match expected (%s)",
	      un, Carol);
    user_unref(carol, "for pointer being discarded");
}

Test(user_suite, create_many, .timeout = 5) {
#ifdef NO_USER
    cr_assert_fail("User module was not implemented");
#endif
    for(int i = 0; i < NUSER; i++) {
	char handle[10];
	sprintf(handle, "U%d", i);
	USER *user = user_create(handle);
	cr_assert_not_null(user, "Returned value was NULL");
    }
}

/*
 * Concurrency test: Create a user, then create a number of threads to increment and
 * decrement its reference count, while holding one reference in reserve so that the
 * user will not be freed. Once the threads have finished, release the final reference
 * and make sure there has been no crash.
 */

static void *ref_thread(void *arg) {
    USER *user = arg;
    unsigned int seed = 1;
    int balance = 0;
    for(int i = 0; i < NITER; i++) {
	int dir = 2 * (rand_r(&seed) % 2) - 1;
	if(balance + dir < 0)
	    dir = -dir;
	if(dir > 0)
	    user_ref(user, NULL);
	else
	    user_unref(user, NULL);
	balance += dir;
    }
    // Drain off any excess balance.
    while(balance--)
	user_unref(user, NULL);
    return NULL;
}

Test(user_suite, concurrent_ref, .timeout = 15) {
#ifdef NO_USER
    cr_assert_fail("User module was not implemented");
#endif
    char *name = "Alice";
    USER *user = user_create(name);
    cr_assert_not_null(user, "Returned value was NULL");
    // Now holding one reference.

    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++)
	pthread_create(&tid[i], NULL, ref_thread, user);

    // Wait for all threads to finish.
    for(int i = 0; i < NTHREAD; i++)
	pthread_join(tid[i], NULL);

    // Release the final reference.
    user_unref(user, "which should be the last reference");
}
