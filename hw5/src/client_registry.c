#include <stdlib.h>
#include <debug.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <stdbool.h>
#include "client_helper.h"
#include "client_registry.h"
#include "client_registry_helper.h"

struct creg_node {
    CLIENT *client_obj;
    struct {
        CREG_NODE *prev;
        CREG_NODE *next;
    } links;
};

struct client_registry {
    pthread_mutex_t client_reg_mutex;
    unsigned volatile int client_counts;
    sem_t shutdown_sem;
    CREG_NODE *dummyNode;
};

CREG_NODE *find_client(CLIENT_REGISTRY *cr, CLIENT *target_client) {
    CREG_NODE *cursor = cr->dummyNode->links.next;
    CREG_NODE *result = NULL;
    while (cursor != cr->dummyNode) {
        if (cursor->client_obj == target_client) {
            result = cursor;
            break;
        }
        cursor = cursor->links.next;
    }
    return result;
}

size_t creg_size(CLIENT_REGISTRY *cr) {
    CREG_NODE *cursor = cr->dummyNode->links.next;
    size_t length = 0;
    while (cursor != cr->dummyNode) {
        length++;
        cursor = cursor->links.next;
    }
    return length;
}

CLIENT_REGISTRY *creg_init() {
    CLIENT_REGISTRY *creg = malloc(sizeof(CLIENT_REGISTRY));
    if (creg == NULL) {
        error("malloc on creating new client registry failed.");
        return NULL;
    }
    pthread_mutex_init(&(creg->client_reg_mutex), NULL);
    CREG_NODE *dummyNode = malloc(sizeof(CREG_NODE));
    if (dummyNode == NULL) {
        error("malloc on creating new client registry failed.");
        pthread_mutex_destroy(&(creg->client_reg_mutex));
        free(creg);
        return NULL;
    }
    dummyNode->client_obj = NULL;
    dummyNode->links.prev = dummyNode;
    dummyNode->links.next = dummyNode;
    creg->client_counts = 0;
    sem_init(&(creg->shutdown_sem), 0, 0);
    creg->dummyNode = dummyNode;
    return creg;
}

void creg_fini(CLIENT_REGISTRY *cr) {
    if (cr == NULL) {
        error("Passed in NULL cr in creg_fini.");
        return;
    }
    pthread_mutex_lock(&(cr->client_reg_mutex));
    debug("SANITY CHECK creg %d", cr->dummyNode->links.next == cr->dummyNode);
    free(cr->dummyNode);
    pthread_mutex_unlock(&(cr->client_reg_mutex));
    pthread_mutex_destroy(&(cr->client_reg_mutex));
    free(cr);
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
    if (cr == NULL || cr < 0) {
        error("Passed in NULL cr or negative fd in creg_register.");
        return NULL;
    }
    pthread_mutex_lock(&(cr->client_reg_mutex));
    CREG_NODE *new_node = malloc(sizeof(CREG_NODE));
    if (new_node == NULL) {
        error("Failed to register new client in client_register");
        goto end;
    }
    CLIENT *result = client_create(cr, fd);
    if (result == NULL) {
        error("Failed to create new client.");
        goto end;
    }
    new_node->client_obj = client_ref(result, "Added to CLIENT_REGISTRY when attenping to register a cnew client.");
    new_node->links.prev = cr->dummyNode->links.prev;
    new_node->links.next = cr->dummyNode;
    cr->dummyNode->links.prev->links.next = new_node;
    cr->dummyNode->links.prev = new_node;
    cr->client_counts++;
end:
    pthread_mutex_unlock(&(cr->client_reg_mutex));
    return result;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {
    if (cr == NULL || client == NULL) {
        error("Passed in NULL cr or NULL client in creg_unregister.");
        return -1;
    }
    pthread_mutex_lock(&(cr->client_reg_mutex));
    CREG_NODE *unregister_node = find_client(cr, client);
    if (unregister_node == NULL) {
        error("cannot find the target client in our client_registry during creg_unregister.");
        pthread_mutex_unlock(&(cr->client_reg_mutex));
        return -1;
    }
    unregister_node->links.prev->links.next = unregister_node->links.next;
    unregister_node->links.next->links.prev = unregister_node->links.prev;
    client_unref(unregister_node->client_obj, "Being remove from creg_unregister.");
    cr->client_counts--;
    debug("client %p unregistered from client_registry", client);
    free(unregister_node);
    if (cr->client_counts == 0) {
        sem_post(&(cr->shutdown_sem));
    }
    pthread_mutex_unlock(&(cr->client_reg_mutex));
    return 0;
}

CLIENT **creg_all_clients(CLIENT_REGISTRY *cr) {
    if (cr == NULL) {
        error("Passed in NULL cr or in creg_all_clients.");
        return NULL;
    }
    pthread_mutex_lock(&(cr->client_reg_mutex));
    int length = creg_size(cr);
    info("Amount of client read from creg_all_clients: %d", length);
    CLIENT **address = malloc((length + 1) * sizeof(CLIENT *));
    if (address == NULL) {
        error("cannot malloc new space to store all the client into the result in creg_all_clients.");
        pthread_mutex_unlock(&(cr->client_reg_mutex));
        return NULL;
    }
    CREG_NODE *cursor = cr->dummyNode->links.next;
    int index = 0;
    while (cursor != cr->dummyNode) {
        address[index] = client_ref(cursor->client_obj, "Increased as called in creg_all_clients.");
        index++;
        cursor = cursor->links.next;
    }
    address[index] = NULL;
    pthread_mutex_unlock(&(cr->client_reg_mutex));
    return address;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    if (cr == NULL) {
        error("Passed in NULL cr or in creg_all_clients.");
        return;
    }
    CREG_NODE *cursor = cr->dummyNode->links.next;
    while (cursor != cr->dummyNode) {
        debug("Shuting down client %p", cursor->client_obj);
        CREG_NODE *current = cursor;
        cursor = cursor->links.next;
        if (shutdown(client_get_fd(current->client_obj), SHUT_RDWR) < 0) {
            error("error shuting down client %p", cursor->client_obj);
        }
    }
    sem_wait(&(cr->shutdown_sem));
}
