#include <stdlib.h>
#include <debug.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include "globals.h"
#include "protocol.h"
#include "user.h"
#include "user_registry.h"
#include "client.h"
#include "client_helper.h"
#include "client_registry.h"
#include "mailbox.h"
#include "server.h"

void *chla_client_service(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    if (pthread_detach(pthread_self()) < 0) {
        error("client_service_thread was not able to make itself detatch.");
        goto end_of_func;
    }
    CLIENT *client = creg_register(client_registry, client_fd);
    if (client == NULL) {
        error("Failed to register client in client_service_thread for fd: %d", client_fd);
        goto end_of_func;
    }
    info("Client service thread is starting for client %p", client);
    CHLA_PACKET_HEADER *packet = calloc(1, sizeof(CHLA_PACKET_HEADER));
    void **payload = calloc(1, sizeof(void *));
    pthread_t mailbox_tid = 0;

    while (proto_recv_packet(client_fd, packet, payload) == 0) {
        uint32_t payload_length = ntohl(packet->payload_length);
        char *message = (char *)*payload;
        for (int i = 0; i < payload_length; i++) {
            debug("%d -> %x", i, message[i]);
        }
        switch (packet->type) {
        case CHLA_LOGIN_PKT: {
            payload_length -= 2; // removed the \r\n termination sequence
            if (payload_length <= 0) {
                debug("No login username.");
                break;
            }
            char *username = malloc(payload_length + 1);
            memcpy(username, message, (size_t)payload_length);
            username[payload_length] = '\0';
            debug("login username: %s", username);
            if (client_login(client, username) == 0) {
                int ret_status = 0;
                CLIENT **malloc_client = malloc(sizeof(CLIENT *));
                *malloc_client = client;
                if ((ret_status = pthread_create(&mailbox_tid, NULL, chla_mailbox_service, malloc_client)) < 0) {
                    debug("cannot create the mailbox_service_thread\n");
                    client_send_nack(client, ntohl(packet->msgid));
                    free(malloc_client); // doesn't need as it will be free by the thread function
                }
                client_send_ack(client, ntohl(packet->msgid), NULL, 0);
            } else {
                client_send_nack(client, ntohl(packet->msgid));
            }
            free(username);
            break;
        }
        case CHLA_LOGOUT_PKT: {
            if (client_logout(client) == 0) {
                pthread_join(mailbox_tid, NULL);
                client_send_ack(client, ntohl(packet->msgid), NULL, 0);
            } else {
                client_send_nack(client, ntohl(packet->msgid));
            }
            break;
        }
        case CHLA_USERS_PKT: {
            CLIENT **list = creg_all_clients(client_registry);
            CLIENT **cursor = list;
            char *new_payload = NULL;
            uint32_t current_size = 0;
            while (*cursor != NULL) {
                if (client_get_login(*cursor)) {
                    USER *user = client_get_user(*cursor, 1);
                    if (user == NULL) {
                        goto skipped;
                    }
                    char *handle = user_get_handle(user);
                    if (handle == NULL) {
                        goto skipped;
                    }
                    debug("obtained handle %s", handle);
                    size_t length = strlen(handle) + 2;
                    new_payload = realloc(new_payload, current_size + length);
                    memcpy(new_payload + current_size, handle, length - 2);
                    new_payload[current_size + length - 1] = '\n';
                    new_payload[current_size + length - 2] = '\r';
                    current_size += length;
                }
            skipped:
                client_unref(*cursor, "Parsed in users command.");
                cursor++;
            }
            client_send_ack(client, ntohl(packet->msgid), new_payload, current_size);
            free(new_payload);
            free(list);
            break;
        }
        case CHLA_SEND_PKT: {
            if (!client_get_login(client)) {
                client_send_nack(client, packet->msgid);
                break;
            }
            char *handle = user_get_handle(client_get_user(client, 1));
            size_t length_of_handle = strlen(handle);
            uint32_t payload_length = ntohl(packet->payload_length);
            char *reciever_handle;
            char *cursor = *payload;
            int read_length = 0;
            while (payload_length > 0 && (*cursor != '\r' && *(cursor + 1) != '\n')) {
                read_length++;
                payload_length--;
                cursor++;
            }
            read_length += 2;
            payload_length -= 2;
            reciever_handle = malloc(read_length - 2 + 1);
            memset(reciever_handle, 0, read_length - 2 + 1);
            memcpy(reciever_handle, *payload, read_length - 2);
            reciever_handle[read_length - 2] = '\0';

            char *new_body = malloc(payload_length + length_of_handle + 2);
            memset(new_body, 0, payload_length + length_of_handle + 2);
            memcpy(new_body, handle, length_of_handle); // copy the username of sender
            memcpy((new_body + length_of_handle), "\r\n", 2); // add the termination sequence
            memcpy((new_body + length_of_handle + 2), (*payload + read_length), payload_length); // copy the rest of the content
            uint32_t new_payload_length = htonl(length_of_handle + 2 + payload_length);

            MAILBOX *from = client_get_mailbox(client, 1);
            CLIENT *recieve = find_client_by_handle(reciever_handle);
            if (recieve == NULL) {
                debug("Cannot find recipiant mailbox.");
                free(reciever_handle);
                free(new_body);
                client_send_nack(client, packet->msgid);
                break;
            }
            MAILBOX *to = client_get_mailbox(recieve, 0);
            if (to == NULL) {
                debug("Cannot find recipiant mailbox.");
                free(reciever_handle);
                free(new_body);
                client_send_nack(client, packet->msgid);
                break;
            }
            mb_add_message(to, packet->msgid, from, new_body, new_payload_length);
            free(new_body);
            mb_unref(to, "Free in CHLA_SEND_PKT.");
            free(reciever_handle);
            client_send_ack(client, packet->msgid, NULL, 0);
            break;
        }
        default: // not used.
            break;
        }
        if (payload_length > 0) {
            free(*payload);
        }
    }
    free(packet);
    free(payload);
    if (client_get_login(client) && client_logout(client) == 0) {
        pthread_join(mailbox_tid, NULL);
    }
    client_unref(client, "Client Service Thread is Terminating.");
    creg_unregister(client_registry, client);
end_of_func:
    close(client_fd);
    debug("Client service thread is shutting down.");
    return NULL;
}

void *chla_mailbox_service(void *arg) {
    CLIENT *client = *(CLIENT **)arg;
    debug("Mailbox service thread for client %p is starting", client);
    free(arg);
    MAILBOX *client_mailbox = client_get_mailbox(client, 0);
    MAILBOX_ENTRY *entry;
    while ((entry = mb_next_entry(client_mailbox)) != NULL) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        switch (entry->type) {
        case MESSAGE_ENTRY_TYPE: {
            CHLA_PACKET_HEADER *new_message = calloc(1, sizeof(CHLA_PACKET_HEADER));
            new_message->type = CHLA_MESG_PKT;
            new_message->msgid = entry->content.message.msgid;
            new_message->payload_length = entry->content.message.length;
            new_message->timestamp_sec = htonl(ts.tv_sec);
            new_message->timestamp_nsec = htonl(ts.tv_nsec);
            if (client_send_packet(client, new_message, entry->content.message.body) < 0) {
                mb_add_notice(entry->content.message.from, BOUNCE_NOTICE_TYPE, entry->content.message.msgid);
            } else {
                mb_add_notice(entry->content.message.from, RRCPT_NOTICE_TYPE, entry->content.message.msgid);
            }
            free(new_message);
            free(entry->content.message.body);
            break;
        }
        case NOTICE_ENTRY_TYPE: {
            CHLA_PACKET_HEADER *new_message = calloc(1, sizeof(CHLA_PACKET_HEADER));
            switch (entry->content.notice.type) {
            case BOUNCE_NOTICE_TYPE:
                new_message->type = CHLA_BOUNCE_PKT;
                break;
            case RRCPT_NOTICE_TYPE:
                new_message->type = CHLA_RCVD_PKT;
                break;
            default: // not used.
                break;
            }
            new_message->msgid = entry->content.notice.msgid;
            new_message->payload_length = htonl(0);
            new_message->timestamp_sec = htonl(ts.tv_sec);
            new_message->timestamp_nsec = htonl(ts.tv_nsec);
            client_send_packet(client, new_message, NULL);
            free(new_message);
            break;
        }
        default: // not used
            break;
        }
        free(entry);
    }
    mb_unref(client_mailbox, "Terminated the client mailbox service thread.");
    return NULL;
}
