#include <stdbool.h>
#ifndef MAILBOX_HELPER_H
#define MAILBOX_HELPER_H

typedef struct mailbox MAILBOX;
typedef struct mailbox_entry MAILBOX_ENTRY;
typedef struct mailbox_node MAILBOX_NODE;

/**
 * @brief the discard_hook_function
 * 
 * @param entry 
 *      the entry of the mailbox.
 */
void discard_hook_general(MAILBOX_ENTRY *entry);

/**
 * @brief enqueue a new message to the mailbox.
 *
 * @param mb
 *     the input mailbox
 * @param entry
 *      the input entry
 * @return true
 *      successfully enqueue
 * @return false
 *      did not funciton successfully
 */
bool mb_enqueue_entry(MAILBOX *mb, MAILBOX_ENTRY *entry);

#endif
