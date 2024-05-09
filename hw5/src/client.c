#include <stdlib.h>
#include <debug.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "globals.h"
#include "client.h"
#include "client_helper.h"
#include "user_registry_helper.h"

// login and logout log, ensure only 1 user can login or out at the same time.
static pthread_mutex_t client_login_logout_lock = PTHREAD_MUTEX_INITIALIZER;

struct client {
    USER *user;
    MAILBOX *mailbox;
    int fd;
    bool is_login;
    unsigned volatile int ref_count;
    pthread_mutex_t mutex;
};

CLIENT *client_create(CLIENT_REGISTRY *creg, int fd) {
    if (fd < 0) {
        error("fd is negative.");
        return NULL;
    }
    CLIENT *new_client = malloc(sizeof(struct client));
    if (new_client == NULL) {
        error("malloc on creating client failed.");
        return NULL;
    }
    pthread_mutex_init(&(new_client->mutex), NULL);
    new_client->fd = fd;
    new_client->user = NULL;
    new_client->mailbox = NULL;
    new_client->is_login = false;
    new_client->ref_count = 1;
    return new_client;
}

CLIENT *client_ref(CLIENT *client, char *why) {
    if (client == NULL) {
        debug("passed in NULL client in client_ref.");
        return NULL;
    }
    pthread_mutex_lock(&(client->mutex));
    info("Increased client %p reference counter by 1 (%d -> %d), reason: %s", client, client->ref_count, client->ref_count + 1, why);
    client->ref_count++;
    pthread_mutex_unlock(&(client->mutex));
    return client;
}

void client_unref(CLIENT *client, char *why) {
    if (client == NULL) {
        error("passed in NULL client in client_ref.");
        return;
    }
    pthread_mutex_lock(&(client->mutex));
    info("Decreased client %p reference counter by 1 (%d -> %d), reason: %s", client, client->ref_count, client->ref_count - 1, why);
    client->ref_count--;
    if (client->ref_count == 0) {
        info("%p is now being free.", client);
        pthread_mutex_unlock(&(client->mutex));
        pthread_mutex_destroy(&(client->mutex));
        free(client);
    } else {
        pthread_mutex_unlock(&(client->mutex));
    }
}

int client_login(CLIENT *client, char *handle) {
    if (client == NULL) {
        error("passed in NULL client in client_login.");
        return -1;
    }
    if (handle == NULL) {
        error("passed in NULL handle in client_login.");
        return -1;
    }
    pthread_mutex_lock(&(client_login_logout_lock));
    CLIENT **all_client = creg_all_clients(client_registry);
    CLIENT **cursor = all_client;
    bool is_good = true;
    while (*cursor != NULL) {
        if (client_get_login(*cursor)) {
            USER *user = client_get_user(*cursor, 1);
            if (user != NULL) {
                char *user_handle = user_get_handle(user);
                if (strcmp(handle, user_handle) == 0) {
                    error("%p attempted to login but another client is already login.", client);
                    is_good = false;
                }
            }
        }
        client_unref(*cursor, "Parsed in client_login.");
        cursor++;
    }
    free(all_client);
    if (!is_good) {
        pthread_mutex_unlock(&(client_login_logout_lock));
        return -1;
    }
    pthread_mutex_lock(&(client->mutex));
    if (client->is_login) {
        error("%p attempted to login but is already login", client);
        pthread_mutex_unlock(&(client_login_logout_lock));
        pthread_mutex_unlock(&(client->mutex));
        return -1;
    }
    USER *user = ureg_register(user_registry, handle);
    if (user == NULL) {
        error("%p attempted get user for the handle but failed.", client);
        pthread_mutex_unlock(&(client_login_logout_lock));
        pthread_mutex_unlock(&(client->mutex));
        return -1;
    }
    // other CLIENT that is logged under the specified handle.
    MAILBOX *mailbox = mb_init(handle);
    if (mailbox == NULL) {
        error("%p attempted create mailbox for the handle but failed", client);
        pthread_mutex_unlock(&(client_login_logout_lock));
        pthread_mutex_unlock(&(client->mutex));
        return -1;
    }
    client->user = user;
    client->mailbox = mailbox;
    client->is_login = true;
    info("Client %p with username \"%s\" has now logged in", client, handle);
    pthread_mutex_unlock(&(client->mutex));
    pthread_mutex_unlock(&(client_login_logout_lock));
    return 0;
}

int client_logout(CLIENT *client) {
    if (client == NULL) {
        error("passed in NULL client in client_logout.");
        return -1;
    }
    pthread_mutex_lock(&(client_login_logout_lock));
    pthread_mutex_lock(&(client->mutex));
    if (!client->is_login) {
        error("client %p is not logged in when attempting to logout.", client);
        pthread_mutex_unlock(&(client_login_logout_lock));
        pthread_mutex_unlock(&(client->mutex));
        return -1;
    }
    client->is_login = false;
    user_unref(client->user, "Logging out of this user.");
    client->user = NULL;
    mb_shutdown(client->mailbox);
    mb_unref(client->mailbox, "Client logged out of the current user and therefore mailbox is getting discarded.");
    client->mailbox = NULL;
    pthread_mutex_unlock(&(client->mutex));
    pthread_mutex_unlock(&(client_login_logout_lock));
    return 0;
}

USER *client_get_user(CLIENT *client, int no_ref) {
    if (client == NULL) {
        error("passed in NULL client in client_get_user.");
        return NULL;
    }
    USER *result = NULL;
    pthread_mutex_lock(&(client->mutex));
    result = client->user;
    pthread_mutex_unlock(&(client->mutex));
    if (result == NULL) {
        return NULL;
    }
    if (no_ref != 0) {
        return result;
    }
    return user_ref(result, "client_get_user is called with a zero no_ref and therefore ref count increment.");
}

MAILBOX *client_get_mailbox(CLIENT *client, int no_ref) {
    if (client == NULL) {
        error("passed in NULL client in client_get_mailbox.");
        return NULL;
    }
    MAILBOX *result = NULL;
    pthread_mutex_lock(&(client->mutex));
    result = client->mailbox;
    pthread_mutex_unlock(&(client->mutex));
    if (result == NULL) {
        return NULL;
    }
    if (no_ref != 0) {
        return result;
    }
    mb_ref(result, "client_get_mailbox is called with a zero no_ref and therefore ref count increment.");
    return result;
}

int client_get_fd(CLIENT *client) {
    if (client == NULL) {
        error("passed in NULL client in client_get_fd.");
        return -1;
    }
    return client->fd;
}

bool client_get_login(CLIENT *client) {
    if (client == NULL) {
        error("passed in NULL client in client_get_login.");
        return false;
    }
    bool result = false;
    pthread_mutex_lock(&(client->mutex));
    result = client->is_login;
    pthread_mutex_unlock(&(client->mutex));
    return result;
}

CLIENT *find_client_by_handle(char *target_handle) {
    if (target_handle == NULL) {
        error("passed in NULL target_handle in find_client.");
        return NULL;
    }
    CLIENT *result = NULL;
    pthread_mutex_lock(&(client_login_logout_lock));
    CLIENT **all_client = creg_all_clients(client_registry);
    CLIENT **cursor = all_client;
    while (*cursor != NULL) {
        if (client_get_login(*cursor)) {
            USER *user = client_get_user(*cursor, 1);
            if (user != NULL) {
                char *user_handle = user_get_handle(user);
                if (strcmp(target_handle, user_handle) == 0) {
                    result = *cursor;
                }
            }
        }
        client_unref(*cursor, "Parsed in client_login.");
        cursor++;
    }
    free(all_client);
    pthread_mutex_unlock(&(client_login_logout_lock));
    return result;
}

int client_send_packet(CLIENT *user, CHLA_PACKET_HEADER *pkt, void *data) {
    if (user == NULL || pkt == NULL) {
        error("passed in NULL client and pkt in client_send_packet.");
        return -1;
    }
    pthread_mutex_lock(&(user->mutex));
    int return_status = proto_send_packet(client_get_fd(user), pkt, data);
    pthread_mutex_unlock(&(user->mutex));
    return return_status;
}

int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen) {
    if (client == NULL) {
        error("passed in NULL client and pkt in client_send_ack.");
        return -1;
    }
    CHLA_PACKET_HEADER *new_packet = malloc(sizeof(CHLA_PACKET_HEADER));
    memset(new_packet, 0, sizeof(CHLA_PACKET_HEADER));
    if (new_packet == NULL) {
        error("passed in malloc'd in client_send_ack failed.");
        return -1;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    new_packet->type = CHLA_ACK_PKT;
    // new_packet->type = htons(CHLA_ACK_PKT);
    new_packet->payload_length = htonl(datalen);
    new_packet->msgid = htonl(msgid);
    new_packet->timestamp_sec = htonl(ts.tv_sec);
    new_packet->timestamp_nsec = htonl(ts.tv_nsec);
    int return_status = client_send_packet(client, new_packet, data);
    free(new_packet);
    return return_status;
}

int client_send_nack(CLIENT *client, uint32_t msgid) {
    if (client == NULL) {
        error("passed in NULL client and pkt in client_send_nack.");
        return -1;
    }
    CHLA_PACKET_HEADER *new_packet = malloc(sizeof(CHLA_PACKET_HEADER));
    memset(new_packet, 0, sizeof(CHLA_PACKET_HEADER));
    if (new_packet == NULL) {
        error("passed in malloc'd in client_send_nack failed.");
        return -1;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    new_packet->type = CHLA_NACK_PKT;
    // new_packet->type = htons(CHLA_NACK_PKT);
    new_packet->payload_length = htonl(0);
    new_packet->msgid = htonl(msgid);
    new_packet->timestamp_sec = htonl(ts.tv_sec);
    new_packet->timestamp_nsec = htonl(ts.tv_nsec);
    int return_status = client_send_packet(client, new_packet, NULL);
    free(new_packet);
    return return_status;
}
