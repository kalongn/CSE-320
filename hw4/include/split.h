#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "debug.h"

#define MAX_COMMAND_TYPE_LENGTH 11
#define AMOUNT_OF_COMMAND 9

#define HELP_COMMAND 0
#define QUIT_COMMAND 1
#define REGISTER_COMMAND 2
#define UNREGISTER_COMMAND 3
#define STATUS_COMMAND 4
#define STATUS_ALL_COMMAND 5
#define START_COMMAND 6
#define STOP_COMMAND 7
#define LOGROTATE_COMMAND 8

extern char *all_command[];

/**
 * @brief Determine what type of command this is, will read until NULL character, or space, or single quatation mark.
 *
 * @param argv
 *      the argv read from a getline. (NOTE: this will be modified by this function)
 * @param length
 *      the length of argv. (NOTE: this will be modified by this function to be either [0,11])
 * @return int
 *      -2: Malloc'd failure.
 *      -1: not a valid string, 0-8 correspond to a mapping
 *      "help" -> 0 |
 *      "quit" -> 1 |
 *      "register" -> 2 |
 *      "unregister" -> 3 |
 *      "status" -> 4 |
 *      "status-all" -> 5 |
 *      "start" -> 6 |
 *      "stop" -> 7 |
 *      "logrotate" -> 8
 */
int read_type_of_command(char *argv, int *length);

/**
 * @brief Split the argument provided into the legion command, Split everything by space, then remove single quote.
 *
 * @param result
 *      either be a NULL pointer on init or prev alloc'd memory, it should be the address of a 2D array
 * @param argv
 *      the argv recieved from a getline command.
 * @param length
 *      the argc pass in doesn't matter, it will record the amount of argument argv (NULL) when funciton exit.
 *
 * @return int
 *      -1 system error, 0: normal
 *
 */
int split_argument(char ***result, char *argv, int *length);