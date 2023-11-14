#include <stdlib.h>

#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */
int validargs(int argc, char **argv)
{
    //[["aa"], ["-f"]...]

    // TO BE IMPLEMENTED.
    if(argc == 1){
        global_options |= 0;
        return 0;
    }

    if (argc > 0)
    {
        if(argc < 5){
                char *tempSecondArgument = *(argv + 1);
                if(*(tempSecondArgument) == '-' && *(tempSecondArgument+1) == 'h' && *(tempSecondArgument+2) == '\0'){
                    global_options |= HELP_OPTION;
                    return 0;
                }
                else if(*(tempSecondArgument) == '-' && *(tempSecondArgument+1) == 'm' && *(tempSecondArgument+2) == '\0'){
                    if(argc == 2){
                        global_options |= MATRIX_OPTION;
                        return 0;
                    }
                }
                else if(*(tempSecondArgument) == '-' && *(tempSecondArgument+1) == 'n' && *(tempSecondArgument+2) == '\0'){
                    if(argc == 2){
                        global_options |= NEWICK_OPTION;
                        return 0;
                    }
                    char *tempThirdArgument = *(argv + 2);
                    if(*(tempThirdArgument) == '-' && *(tempThirdArgument+1) == 'o' && *(tempThirdArgument+2) == '\0'){
                        if(argc == 4){
                            global_options |= NEWICK_OPTION;
                            outlier_name = *(argv + 3);
                            return 0;
                        }
                    }
                }

        }

    }
    return -1;

}
