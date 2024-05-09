#include <stdlib.h>
#include <stdbool.h>
#include <debug.h>
#include <pthread.h>
#include <string.h>
#include "user.h"

struct user {
    char *handle;
    unsigned volatile int ref_count;
    // volatile bool is_checked_in;
    pthread_mutex_t mutex;
};

USER *user_create(char *handle) {
    if (handle == NULL) {
        error("Passed in handle is NULL for user_create.");
        return NULL;
    }
    USER *new_user = calloc(1, sizeof(USER));
    if (new_user == NULL) {
        error("calloc on creating user failed.");
        return NULL;
    }
    pthread_mutex_init(&(new_user->mutex), NULL);
    new_user->ref_count = 1;
    new_user->handle = strdup(handle);
    if (new_user->handle == NULL) {
        pthread_mutex_destroy(&(new_user->mutex));
        free(new_user);
        error("strdup on creating user handle failed.");
        return NULL;
    }
    // new_user->is_checked_in = false;
    return new_user;
}

USER *user_ref(USER *user, char *why) {
    if (user == NULL) {
        error("passed in NULL user in user_ref.");
        return NULL;
    }
    pthread_mutex_lock(&(user->mutex));
    info("Increased user %p reference counter by 1 (%d -> %d), reason: %s", user, user->ref_count, user->ref_count + 1, why);
    user->ref_count++;
    pthread_mutex_unlock(&(user->mutex));
    return user;
}

void user_unref(USER *user, char *why) {
    if (user == NULL) {
        error("passed in NULL user in user_unref.");
        return;
    }
    pthread_mutex_lock(&(user->mutex));
    info("Decreased user %p reference counter by 1 (%d -> %d), reason: %s", user, user->ref_count, user->ref_count - 1, why);
    user->ref_count--;
    if (user->ref_count == 0) {
        info("%p is now being free.", user);
        free(user->handle);
        pthread_mutex_unlock(&(user->mutex));
        pthread_mutex_destroy(&(user->mutex));
        free(user);
    } else {
        pthread_mutex_unlock(&(user->mutex));
    }
}

char *user_get_handle(USER *user) {
    if (user == NULL) {
        error("ERROR: passed in NULL user in user_get_handle.");
        return NULL;
    }
    return user->handle;
}
