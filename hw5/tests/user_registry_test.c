#include <criterion/criterion.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "client_registry.h"
#include "user_registry.h"
#include "globals.h"
#include "excludes.h"

/* The maximum number of users we will register. */
#define NUSER (3000)

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (10)

/* Number of iterations we use in several tests. */
#define NITER (1000000)

static void init() {
    // Nothing for now.
}

/*
 * Initialize and immediately finalize a registry, to see if any crash.
 */
Test(user_registry_suite, init_fini, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    ureg_fini(ur);
}

/*
 * Test one registry, one thread registering a single user.
 */
Test(user_registry_suite, reg_one_registry, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Register a user.
    char *name = "Alice";
    USER *user = ureg_register(ur, name);
    cr_assert_not_null(user, "Returned user was NULL");

    // Check the users name.
    char *handle = user_get_handle(user);
    cr_assert(!strcmp(handle, name),
	      "Handle of returned user (%s) did not match expected (%d)",
	      handle, name);
}

/*
 * Initialize a registry, register a user and finalize the registry without
 * unregistering the user.
 */
Test(user_registry_suite, init_reg_fini, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Register a user.
    char *name = "Alice";
    USER *user = ureg_register(ur, name);
    cr_assert_not_null(user, "Returned user was NULL");

    ureg_fini(ur);
}

/*
 * Test one registry, register the same user twice and check that the
 * same object is returned.
 */
Test(user_registry_suite, rereg_one_registry, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Register a user.
    char *name = "Alice";
    USER *user = ureg_register(ur, name);
    cr_assert_not_null(user, "Returned user was NULL");

    // Register the same handle again.
    USER *user1 = ureg_register(ur, name);

    // Check that the same user was returned.
    cr_assert_eq(user, user1, "The returned users were not the same (%p != %p)",
		 user, user1);
}

/*
 * Test one registry, register a user, release one reference, register the
 * same user again and check that the same object is returned and that the
 * handle is correct.
 */
Test(user_registry_suite, release_rereg_one_registry, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Register a user.
    char *name = "Alice";
    USER *user = ureg_register(ur, name);
    cr_assert_not_null(user, "Returned user was NULL");

    // Release one reference.
    user_unref(user, "for pointer being discarded");

    // Register the same handle again.
    USER *user1 = ureg_register(ur, name);

    // Check that the same user was returned.
    cr_assert_eq(user, user1, "The returned users were not the same (%p != %p)",
		 user, user1);

    // Check the users name.
    char *handle = user_get_handle(user);
    cr_assert(!strcmp(handle, name),
	      "Handle of returned user (%s) did not match expected (%d)",
	      handle, name);
}

/*
 * Register a few users and check the results.
 */
Test(user_registry_suite, reg3_one_registry, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Register some users and check their names.
    char *Alice = "Alice";
    USER *alice = ureg_register(ur, Alice);
    cr_assert_not_null(alice, "Returned user was NULL");
    char *handle = user_get_handle(alice);
    cr_assert(!strcmp(handle, Alice),
	      "Handle of returned user (%s) did not match expected (%d)",
	      handle, Alice);

    char *Bob = "Bob";
    USER *bob = ureg_register(ur, Bob);
    cr_assert_not_null(bob, "Returned user was NULL");
    handle = user_get_handle(bob);
    cr_assert(!strcmp(handle, Bob),
	      "Handle of returned user (%s) did not match expected (%d)",
	      handle, Bob);

    char *Carol = "Carol";
    USER *carol = ureg_register(ur, Carol);
    cr_assert_not_null(carol, "Returned user was NULL");
    handle = user_get_handle(carol);
    cr_assert(!strcmp(handle, Carol),
	      "Handle of returned user (%s) did not match expected (%d)",
	      handle, Carol);
}

/*
 * Register a user, release a reference, and then unregister the user.
 */
Test(user_registry_suite, reg_release_unreg_one_registry, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Register a user.
    char *name = "Alice";
    USER *user = ureg_register(ur, name);
    cr_assert_not_null(user, "Returned user was NULL");

    // Release one reference.
    user_unref(user, "for pointer being discarded");

    // Unregister the handle.
    ureg_unregister(ur, name);
}

// Unregister from an empty registry to check for a crash.
Test(user_registry_suite, unreg_empty_one_registry, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Unregister a user.
    char *name = "Alice";
    ureg_unregister(ur, name);
}

// Test registering different users in two registries.
// This is basically to check for inappropriate static variables.
Test(user_registry_suite, reg_two_registries, .init = init, .timeout = 5) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur1 = ureg_init();
    cr_assert_not_null(ur1);
    USER_REGISTRY *ur2 = ureg_init();
    cr_assert_not_null(ur2);

    // Register a user in each registry and check their names.
    char *Alice = "Alice";
    USER *alice1 = ureg_register(ur1, Alice);
    cr_assert_not_null(alice1, "Returned user was NULL");
    char *handle = user_get_handle(alice1);
    cr_assert(!strcmp(handle, Alice),
	      "Handle of returned user (%s) did not match expected (%d)",
	      handle, Alice);

    char *Bob = "Bob";
    USER *bob2 = ureg_register(ur2, Bob);
    cr_assert_not_null(bob2, "Returned user was NULL");
    handle = user_get_handle(bob2);
    cr_assert(!strcmp(handle, Bob),
	      "Handle of returned user (%s) did not match expected (%d)",
	      handle, Bob);

    // Now register each user in the other registry and check that the
    // returned object is different.
    USER *alice2 = ureg_register(ur2, Alice);
    cr_assert_not_null(alice2, "Returned user was NULL");
    cr_assert_neq(alice1, alice2, "Users are equal that should not be (%p == %p)",
		  alice1, alice2);

    USER *bob1 = ureg_register(ur1, Bob);
    cr_assert_not_null(bob1, "Returned user was NULL");
    cr_assert_neq(bob1, bob2, "Users are equal that should not be (%p == %p)",
		  bob1, bob2);
}

/*
 * Concurrency test.
 * Several threads perform the following:
 *   Users randomly selected from a large pool are registered concurrently.
 *      A record is kept of the user objects returned and it is checked that at
 *      most one user object is seen for each handle.
 *   The test terminates when every user in the pool has been registered.
 */

/*
 * User objects that have been registered.
 * These are needed in order to unregister.
 */
static USER *users[NUSER];
static int users_count = 0;

/* Mutex to protect the users array and count. */
static pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Names used when registering users.
 */
static char *names[NUSER];

/*
 * Repeatedly register a user randomly selected from the pool.
 * Check the object returned against the users array.
 * If the array entry is NULL, then save the object in the array.
 * If the array entry is not NULL, then check to make sure the object
 * is the same as the one in the array.
 *
 * A mutex is used to protect the users array, but we do not want to
 * hold the mutex while calling the register operation, as that would
 * destroy the concurrency and defeat the purpose of the test.
 */
void random_reg(USER_REGISTRY *ur) {
    unsigned int seed = 1; //pthread_self();
    int iters = NITER;
    int count;
    do {
	int u = rand_r(&seed) % NUSER;
	char *name = names[u];
	USER *user = ureg_register(ur, name);

	// If the user is already registered, we should get back the
	// existing object.
	pthread_mutex_lock(&users_mutex);
	if(users[u] == NULL) {
	    users[u] = user;
	    users_count++;
	} else {
	    cr_assert_eq(user, users[u], "User object is different from that previously returned");
	}
	count = users_count;
	pthread_mutex_unlock(&users_mutex);
    } while(count < NUSER && iters-- > 0);
}

/*
 * Thread that runs random registration until the users array is full.
 */
struct random_reg_args {
    USER_REGISTRY *ur;
};

void *random_reg_thread(void *arg) {
    struct random_reg_args *ap = arg;
    random_reg(ap->ur);
    return NULL;
}

Test(user_registry_suite, concurrency, .init = init, .timeout = 60) {
#ifdef NO_USER_REGISTRY
    cr_assert_fail("User registry was not implemented");
#endif
    USER_REGISTRY *ur = ureg_init();
    cr_assert_not_null(ur);

    // Initialize the pool of names.
    for(int i = 0; i < NUSER; i++) {
	char handle[10];
	sprintf(handle, "u%d", i);
	names[i] = strdup(handle);
    }

    // Spawn threads to run random registration.
    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++) {
	struct random_reg_args *ap = calloc(1, sizeof(struct random_reg_args));
	ap->ur = ur;
	pthread_create(&tid[i], NULL, random_reg_thread, ap);
    }

    // Wait for the threads to finish.
    for(int i = 0; i < NTHREAD; i++)
	pthread_join(tid[i], NULL);
}




// Test that finalization frees existing users.
// Test ureg_fini() on a registry that is supposed to be empty and on a registry that is not empty.
// Check for leaks or errors with valgrind.


