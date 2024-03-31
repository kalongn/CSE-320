#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"
#include "audio.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

AUDIO_HEADER header;
int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & 1)
        USAGE(*argv, EXIT_SUCCESS);

    // FILE * f = fopen("/home/student/Desktop/snahian/hw1/all941.au", "r");
    // audio_read_header(f, &header);

    int generate_command_used = (global_options & 2) >> 1;
    if (generate_command_used) {
        // -g flag was used
        if (dtmf_generate(stdin, stdout, audio_samples) == EOF) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    int detect_command_used = (global_options & 4) >> 2;
    if (detect_command_used) {
        // the -d flag was used
            // printf("WHAT\n");
        if (dtmf_detect(stdin, stdout) == EOF) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // means that another command was used (should never reach this point because of the validation done at validate args)
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
