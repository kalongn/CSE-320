#include "split.h"


char *all_command[] = {
    "help", // 0
    "quit", // 1
    "register", // 2
    "unregister", // 3
    "status", // 4
    "status-all", // 5
    "start", // 6
    "stop", // 7
    "logrotate", // 8
};

int read_type_of_command(char *argv, int *length) {
    char *cursor = strndup(argv, MAX_COMMAND_TYPE_LENGTH + 1);
    if (cursor == NULL) {
        return -2;
    }
    char *token = NULL;
    *length = 0;
    token = strtok(cursor, " ");
    if (token == NULL) {
        free(cursor);
        return -1;
    }
    int read_command_length = strnlen(token, MAX_COMMAND_TYPE_LENGTH + 1);
    *length = read_command_length;
    if (read_command_length == MAX_COMMAND_TYPE_LENGTH + 1 || read_command_length < 4) {
        free(cursor);
        return -1;
    }
    for (int i = 0; i < AMOUNT_OF_COMMAND; i++) {
        if (strcmp(all_command[i], token) == 0) {
            free(cursor);
            return i;
        }
    }
    free(cursor);
    return -1;
}


int split_argument(char ***result, char *argv, int *length) {
    *result = realloc(*result, sizeof(char *));
    if (*result == NULL) {
        return -1;
    }
    char *cursor = argv;
    int argc = 0;
    bool inQuote = false;
    while (*cursor) {
        if (*cursor == 0x20) {
            *cursor = '\0';
            cursor++;
            continue;
        }
        char *begin = cursor;
        while (*cursor && ((*cursor != 0x20 || inQuote))) {
            if (*cursor == 0x27) {
                inQuote = !inQuote;
            }
            cursor++;
        }
        argc++;
        *result = realloc(*result, sizeof(char *) * argc);
        (*result)[argc - 1] = begin;
    }
    for (int i = 0; i < argc; i++) {
        char *shift_cursor = (*result)[i];
        while (*shift_cursor) {
            if (*shift_cursor == 0x27) {
                char *shift_begin = shift_cursor;
                while (*shift_begin) {
                    *shift_begin = *(shift_begin + 1);
                    shift_begin++;
                }
                *shift_begin = '\0';
            } else {
                shift_cursor++;
            }
        }
    }
    *length = argc;
    argc++;
    *result = realloc(*result, sizeof(char *) * argc);
    if (*result == NULL) {
        return -1;
    }
    (*result)[argc - 1] = NULL;
    return 0;
}