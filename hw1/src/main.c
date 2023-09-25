#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "debug.h"

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);
    // TO BE IMPLEMENTED
    if(global_options == 0){
        if(read_distance_data(stdin) != 0){
            printf("%s\n", "failed");
            return EXIT_FAILURE;
        }
        if(build_taxonomy(stdout) == -1){
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if(global_options == NEWICK_OPTION){
        if(read_distance_data(stdin) != 0){
            printf("%s\n", "failed");
            return EXIT_FAILURE;
        }
        if(emit_newick_format(stdout) == -1){
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;

    }
    if(global_options == MATRIX_OPTION){
        if(read_distance_data(stdin) != 0){
            printf("%s\n", "failed");
            return EXIT_FAILURE;
        }
        if(emit_distance_matrix(stdout) == -1){
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
