#include <stdlib.h>
#include <debug.h>
#include <pthread.h>
#include <string.h>
#include "user_registry.h"
#include "user_registry_helper.h"

// doubly cicular linked list
struct ureg_node {
    char *handle;
    USER *user_obj;
    struct {
        UREG_NODE *prev;
        UREG_NODE *next;
    } links;
};

struct user_registry {
    pthread_mutex_t user_reg_mutex;
    UREG_NODE *dummyNode;
};

UREG_NODE *find_handle(USER_REGISTRY *ureg, char *target_handle) {
    UREG_NODE *cursor = ureg->dummyNode->links.next;
    UREG_NODE *result = NULL;
    while (cursor != ureg->dummyNode) {
        if (strcmp(target_handle, cursor->handle) == 0) {
            result = cursor;
            break;
        }
        cursor = cursor->links.next;
    }
    return result;
}

USER_REGISTRY *ureg_init(void) {
    USER_REGISTRY *ureg = calloc(1, sizeof(USER_REGISTRY));
    if (ureg == NULL) {
        error("calloc on creating new user registry failed.");
        return NULL;
    }
    pthread_mutex_init(&(ureg->user_reg_mutex), NULL);
    UREG_NODE *dummyNode = calloc(1, sizeof(UREG_NODE));
    if (dummyNode == NULL) {
        pthread_mutex_destroy(&(ureg->user_reg_mutex));
        free(ureg);
        error("calloc on creating new user registry failed.");
        return NULL;
    }
    dummyNode->handle = NULL;
    dummyNode->user_obj = NULL;
    dummyNode->links.next = dummyNode;
    dummyNode->links.prev = dummyNode;
    ureg->dummyNode = dummyNode;
    return ureg;
}

void ureg_fini(USER_REGISTRY *ureg) {
    if (ureg == NULL) {
        return;
    }
    pthread_mutex_lock(&(ureg->user_reg_mutex));
    UREG_NODE *cursor = ureg->dummyNode->links.next;
    while (cursor != ureg->dummyNode) {
        UREG_NODE *curr = cursor;
        cursor = cursor->links.next;
        free(curr->handle);
        user_unref(curr->user_obj, "Final Deference for ureg_fini.");
        free(curr);
    }
    free(ureg->dummyNode);
    pthread_mutex_unlock(&(ureg->user_reg_mutex));
    pthread_mutex_destroy(&(ureg->user_reg_mutex));
    free(ureg);
}

USER *ureg_register(USER_REGISTRY *ureg, char *handle) {
    pthread_mutex_lock(&(ureg->user_reg_mutex));
    if (ureg == NULL || handle == NULL) {
        return NULL;
    }
    USER *result = NULL;
    UREG_NODE *search_result = find_handle(ureg, handle);
    if (search_result != NULL) {
        result = user_ref(search_result->user_obj, "Found in USER_REGISTRY when attempting to register a new handle.");
    } else {
        UREG_NODE *new_node = calloc(1, sizeof(UREG_NODE));
        if (new_node == NULL) {
            error("calloc on registering a new user in user_reg failed.");
            goto end;
        }
        new_node->handle = strdup(handle);
        if (new_node->handle == NULL) {
            free(new_node);
            error("strdup on creating user handle failed.");
            goto end;
        }
        new_node->user_obj = user_create(handle);
        if (new_node->user_obj == NULL) {
            free(new_node->handle);
            free(new_node);
            error("Cannot create a new user object to be put into User registry.");
            goto end;
        }
        new_node->user_obj = user_ref(new_node->user_obj, "Added to USER_REGISTRY when attemping to register a new handle.");
        new_node->links.prev = ureg->dummyNode->links.prev;
        new_node->links.next = ureg->dummyNode;
        ureg->dummyNode->links.prev->links.next = new_node;
        ureg->dummyNode->links.prev = new_node;
        result = new_node->user_obj;
    }
end:
    pthread_mutex_unlock(&(ureg->user_reg_mutex));
    return result;
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle) {
    if (ureg == NULL || handle == NULL) {
        return;
    }
    pthread_mutex_lock(&(ureg->user_reg_mutex));
    UREG_NODE *search_result = find_handle(ureg, handle);
    if (search_result != NULL) {
        free(search_result->handle);
        user_unref(search_result->user_obj, "Released from USER_REGISTRY.");
        search_result->links.prev->links.next = search_result->links.next;
        search_result->links.next->links.prev = search_result->links.prev;
        free(search_result);
    }
    pthread_mutex_unlock(&(ureg->user_reg_mutex));
}
