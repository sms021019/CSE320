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
        build_taxonomy(stdout);
    }
    if(global_options == NEWICK_OPTION){
        if(read_distance_data(stdin) != 0){
            printf("%s\n", "failed");
            return EXIT_FAILURE;
        }
        emit_newick_format(stdout);
    }
    if(global_options == MATRIX_OPTION){
        if(read_distance_data(stdin) != 0){
            printf("%s\n", "failed");
            return EXIT_FAILURE;
        }
        emit_distance_matrix(stdout);
    }
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
