#ifndef CLIENT_HELPER_H
#define CLIENT_HELPER_H
#include <stdbool.h>

typedef struct client CLIENT;

/**
 * @brief return the login status of the current client.
 *
 * @param client
 *       The CLIENT for which the login_status is to be obtained.
 * @return true
 *      The client is login.
 * @return false
 *      the client is not login,
 */
bool client_get_login(CLIENT *client);

/**
 * @brief Return the address of the client with the target_handle.
 *
 * @param target_handle
 *      the name which a client contains you want to find
 * @return CLIENT*
 *      the CLIENT with the target_handle, NULL if not found or not succesful.
 */
CLIENT *find_client_by_handle(char *target_handle);

#endif
