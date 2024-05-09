#ifndef CLIENT_REGISTRY_HELPER_H
#define CLIENT_REGISTRY_HELPER_H

typedef struct client CLIENT;
typedef struct creg_node CREG_NODE;
typedef struct client_registry CLIENT_REGISTRY;

/**
 * @brief Find the target CLIENT_REGISTRY Node within the list. (THIS SHOULD ONLY BE CALL IF client_reg_mutex is already locked.)
 *
 * @param cr
 *       the reference of the current client_registry.
 * @param target_client
 *      the target client we want to find.
 * @return CREG_NODE*
 *      the CREG_NODE Node that contains the target_client, NULL if not found.
 *
 */
CREG_NODE *find_client(CLIENT_REGISTRY *cr, CLIENT *target_client);

/**
 * @brief Return the amount of client the current CLIENT_REGISTRY has. (THIS SHOULD ONLY BE CALL IF client_reg_mutex is already locked.)
 *
 * @param cr
 *      the reference of the current client_registry.
 *
 * @return size_t
 *      0 if the registry only has dummyNode
 */
size_t creg_size(CLIENT_REGISTRY *cr);

#endif
