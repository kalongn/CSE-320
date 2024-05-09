#ifndef USER_REGISTRY_HELPER_H
#define USER_REGISTRY_HELPER_H

// defined for compile reason
typedef struct ureg_node UREG_NODE;
typedef struct user_registry USER_REGISTRY;

/**
 * @brief Find the target USER_REGISTRY Node within the list. (THIS SHOULD ONLY BE CALL IF user_reg_mutex is already locked.)
 *
 * @param ureg
 *      the "dummyNode" reference of the current ureg.
 * @param target_handle
 *      the target handle name we want to find.
 * @return UREG_NODE*
 *      the pointer to the Node user searched for, or NULL if not found.
 */
UREG_NODE *find_handle(USER_REGISTRY *ureg, char *target_handle);


#endif
