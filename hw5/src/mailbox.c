#include <stdlib.h>
#include <debug.h>
#include <semaphore.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include "mailbox.h"
#include "mailbox_helper.h"

struct mailbox_node {
    MAILBOX_ENTRY *entry;
    struct {
        struct mailbox_node *prev;
        struct mailbox_node *next;
    } links;
};

struct mailbox {
    char *handle;
    unsigned volatile int ref_count;
    bool is_defunt;
    pthread_mutex_t mailbox_mutex;
    sem_t mailbox_semaphore;
    MAILBOX_NODE *dummyNode;
    MAILBOX_DISCARD_HOOK *hook_function;
};

MAILBOX *mb_init(char *handle) {
    if (handle == NULL) {
        error("passed in NULL handle in mb_init.");
        return NULL;
    }
    MAILBOX *new_mailbox = calloc(1, sizeof(MAILBOX));
    if (new_mailbox == NULL) {
        error("malloc failed when attempting to create new mailbox in mb_init.");
        return NULL;
    }
    char *private_handle = strdup(handle);
    if (private_handle == NULL) {
        free(new_mailbox);
        error("malloc failed when attempting to create a private handle in mb_init.");
        return NULL;
    }
    new_mailbox->handle = private_handle;
    pthread_mutex_init(&(new_mailbox->mailbox_mutex), NULL);
    sem_init(&(new_mailbox->mailbox_semaphore), 0, 0); // 0 init value indicate the queue is "empty"
    MAILBOX_NODE *dummyNode = calloc(1, sizeof(MAILBOX_NODE));
    if (dummyNode == NULL) {
        pthread_mutex_destroy(&(new_mailbox->mailbox_mutex));
        sem_destroy(&(new_mailbox->mailbox_semaphore));
        free(new_mailbox->handle);
        free(new_mailbox);
        error("malloc failed when attempting to create a private handle in mb_init.");
        return NULL;
    }
    dummyNode->entry = NULL;
    dummyNode->links.prev = dummyNode;
    dummyNode->links.next = dummyNode;
    new_mailbox->dummyNode = dummyNode;
    new_mailbox->is_defunt = false;
    new_mailbox->ref_count = 1;
    new_mailbox->hook_function = discard_hook_general;
    return new_mailbox;
}

void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK *function) {
    if (mb == NULL) {
        error("passed in NULL mb into mb_set_discard_hook");
        return;
    }
    pthread_mutex_lock(&(mb->mailbox_mutex));
    mb->hook_function = function;
    pthread_mutex_unlock(&(mb->mailbox_mutex));
}

void discard_hook_general(MAILBOX_ENTRY *entry) {
    if (entry->type == MESSAGE_ENTRY_TYPE) {
        if (entry->content.message.from != NULL) {
            mb_add_notice(entry->content.message.from, BOUNCE_NOTICE_TYPE, entry->content.message.msgid);
        }
    }
}

void mb_ref(MAILBOX *mb, char *why) {
    if (mb == NULL) {
        error("passed in NULL mb into mb_ref");
        return;
    }
    if (mb->is_defunt) {
        error("mailbox %p is defunt.", mb);
        return;
    }
    pthread_mutex_lock(&(mb->mailbox_mutex));
    info("Increased mailbox %p reference counter by 1 (%d -> %d), reason: %s", mb, mb->ref_count, mb->ref_count + 1, why);
    mb->ref_count++;
    pthread_mutex_unlock(&(mb->mailbox_mutex));
}

void mb_unref(MAILBOX *mb, char *why) {
    if (mb == NULL) {
        error("passed in NULL mb into mb_ref");
        return;
    }
    pthread_mutex_lock(&(mb->mailbox_mutex));
    info("Decreased mailbox %p reference counter by 1 (%d -> %d), reason: %s", mb, mb->ref_count, mb->ref_count - 1, why);
    mb->ref_count--;
    if (mb->ref_count == 0) {
        info("mailbox %p is now being free.", mb);
        free(mb->handle);
        debug("SANITY CHECK: %d", mb->dummyNode->links.next == mb->dummyNode);
        free(mb->dummyNode);
        sem_destroy(&(mb->mailbox_semaphore));
        pthread_mutex_unlock(&(mb->mailbox_mutex));
        pthread_mutex_destroy(&(mb->mailbox_mutex));
        free(mb);
    } else {
        pthread_mutex_unlock(&(mb->mailbox_mutex));
    }
}

void mb_shutdown(MAILBOX *mb) {
    if (mb == NULL) {
        error("passed in NULL mb into mb_shutdown.");
        return;
    }
    pthread_mutex_lock(&(mb->mailbox_mutex));
    info("Shutting down mailbox %p: \'%s\'", mb, mb_get_handle(mb));
    mb->is_defunt = true;
    pthread_mutex_unlock(&(mb->mailbox_mutex));
    sem_post(&(mb->mailbox_semaphore));
}

char *mb_get_handle(MAILBOX *mb) {
    if (mb == NULL) {
        error("passed in NULL mb into mb_get_handle.");
        return NULL;
    }
    return mb->handle;
}

bool mb_enqueue_entry(MAILBOX *mb, MAILBOX_ENTRY *entry) {
    if (mb == NULL || entry == NULL) {
        error("passed in NULL mb or NULL entry into mb_enqueue_entry.");
        return false;
    }
    pthread_mutex_lock(&(mb->mailbox_mutex));
    if (mb->is_defunt) {
        error("mailbox %p is defunt and an entry was not", mb);
        pthread_mutex_unlock(&(mb->mailbox_mutex));
        return false;
    }
    MAILBOX_NODE *new_node = malloc(sizeof(MAILBOX_NODE));
    if (new_node == NULL) {
        error("cannot malloc a new wrapper object to append the new message to the set in mb_add_message.");
        pthread_mutex_unlock(&(mb->mailbox_mutex));
        return false;
    }
    new_node->links.next = mb->dummyNode;
    new_node->links.prev = mb->dummyNode->links.prev;
    mb->dummyNode->links.prev->links.next = new_node;
    mb->dummyNode->links.prev = new_node;
    new_node->entry = entry;
    pthread_mutex_unlock(&(mb->mailbox_mutex));
    sem_post(&(mb->mailbox_semaphore));
    return true;
}

void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length) {
    if (mb == NULL || from == NULL) {
        error("passed in NULL mb or NULL from into mb_add_message.");
        return;
    }
    MAILBOX_ENTRY *new_message = malloc(sizeof(MAILBOX_ENTRY));
    memset(new_message, 0, sizeof(MAILBOX_ENTRY));
    if (new_message == NULL) {
        error("cannot malloc a new message object in mb_add_message.");
        return;
    }
    if (mb != from) {
        mb_ref(from, "senders mailbox refer increased in mb_add_message.");
    }
    new_message->type = MESSAGE_ENTRY_TYPE;
    new_message->content.message.msgid = msgid;
    new_message->content.message.from = from;
    char *new_body = calloc(1, ntohl(length));
    if (new_body == NULL) {
        error("cannot malloc a new body object in mb_add_message.");
        if (mb != from) {
            mb_unref(from, "senders mailbox refer decreased in mb_add_message.");
        }
        free(new_message);
        return;
    }
    memcpy(new_body, body, ntohl(length));
    new_message->content.message.body = (void *)new_body;
    new_message->content.message.length = length;
    if (!mb_enqueue_entry(mb, new_message)) {
        if (mb != from) {
            mb_unref(from, "senders mailbox refer decreased in mb_add_message.");
        }
        free(new_message);
    }
}

void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid) {
    if (mb == NULL) {
        error("ERROR: passed in NULL mb into mb_add_message.");
        return;
    }
    MAILBOX_ENTRY *new_notice = malloc(sizeof(MAILBOX_ENTRY));
    memset(new_notice, 0, sizeof(MAILBOX_ENTRY));
    if (new_notice == NULL) {
        error("cannot malloc a new notice object in mb_add_message.");
        return;
    }
    new_notice->type = NOTICE_ENTRY_TYPE;
    new_notice->content.notice.type = ntype;
    new_notice->content.notice.msgid = msgid;
    if (!mb_enqueue_entry(mb, new_notice)) {
        free(new_notice);
    }
}

MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb) {
    if (mb == NULL) {
        error("passed in NULL mb into mb_next_entry.");
        return NULL;
    }
    sem_wait(&(mb->mailbox_semaphore));
    pthread_mutex_lock(&(mb->mailbox_mutex));
    if (mb->is_defunt) {
        MAILBOX **need_to_unref = NULL;
        int counter = 0;
        MAILBOX_NODE *cursor = mb->dummyNode->links.next;
        while (cursor != mb->dummyNode) {
            if (mb->hook_function != NULL) {
                if (mb == cursor->entry->content.message.from) {
                    cursor->entry->content.message.from = NULL;
                }
                mb->hook_function(cursor->entry);
            }
            if (cursor->entry->type == MESSAGE_ENTRY_TYPE && cursor->entry->content.message.from != NULL) {
                counter++;
                need_to_unref = realloc(need_to_unref, sizeof(MAILBOX *) * counter);
                need_to_unref[counter - 1] = cursor->entry->content.message.from;
                if (cursor->entry->content.message.body != NULL) {
                    free(cursor->entry->content.message.body);
                }
            }
            free(cursor->entry);
            MAILBOX_NODE *delete_node = cursor;
            cursor = cursor->links.next;
            free(delete_node);
        }
        pthread_mutex_unlock(&(mb->mailbox_mutex));
        for (int i = 0; i < counter; i++) {
            mb_unref(need_to_unref[i], "Message recieved defunt.");
        }
        free(need_to_unref);
        return NULL;
    }
    MAILBOX_NODE *delete_node = mb->dummyNode->links.next;
    MAILBOX_ENTRY *recieved_entry = delete_node->entry;
    delete_node->links.next->links.prev = mb->dummyNode;
    mb->dummyNode->links.next = delete_node->links.next;
    free(delete_node);
    pthread_mutex_unlock(&(mb->mailbox_mutex));
    if (recieved_entry->type == MESSAGE_ENTRY_TYPE && recieved_entry->content.message.from != mb) {
        mb_unref(recieved_entry->content.message.from, "Message recieved normal.");
    }
    return recieved_entry;
}
