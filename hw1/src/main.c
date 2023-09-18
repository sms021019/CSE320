#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "debug.h"

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION){
        printf("%s\n", "Hi");
        USAGE(*argv, EXIT_SUCCESS);
    }
    if(global_options == 0){
        fprintf(stdout, "%s\n", "1");
    }

    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
