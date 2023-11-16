#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "deet.h"
#include "commands.h"
#include "helper.h"
#include "debug.h"


int main(int argc, char *argv[]) {
    int prompt_enabled = 1;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            prompt_enabled = 0; // Disable prompt if -p flag is found
            break;
        }
    }

    log_startup();
    char *input_line = NULL;
    size_t len = 0;
    ssize_t nread;

    struct sigaction sa_chld, sa_int;

    // Set up SIGCHLD handler
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }

    // Set up SIGINT handler
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; // SA_RESTART is usually not used with SIGINT
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    char *args[10];
    int arg_count;

    while (1) {
        log_prompt();
        if (prompt_enabled) {
            printf("deet> ");
            fflush(stdout);
        }

        arg_count = 0;
        nread = getline(&input_line, &len, stdin);

        if (nread == -1) {
            if (errno == EINTR) {
                // System call was interrupted by SIGCHLD, continue to re-prompt
                continue;
            } else {
                // Some other error or EOF, handle or exit as needed
                free(input_line);
                exit(EXIT_SUCCESS);
            }
        }
        log_input(input_line);

        // Remove newline character from input_line
        input_line[strcspn(input_line, "\n")] = 0;

        // Tokenize the input to get the first argument
        char *command = strtok(input_line, " ");

        if (command == NULL) {
            continue;
        }

        char *temp = strtok(NULL, " ");
        while(temp != NULL){
            args[arg_count++] = temp;
            temp = strtok(NULL, " ");
        }

        args[arg_count] = NULL;

        const char *program_name = args[0];

        if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "quit") == 0) {
            quit_command();
            free(input_line);
            log_shutdown();
            break;
        } else if (strcmp(command, "show") == 0) {
            if(print_all_managed_processes() == -1)
                goto PRINTERR;
        } else if (strcmp(command, "run") == 0) {
            if(run_command(program_name, args) == -1)
                goto PRINTERR;
        } else if (strcmp(command, "stop") == 0) {
            if(stop_command(args) == -1)
                goto PRINTERR;
        } else if (strcmp(command, "cont") == 0) {
            if(cont_command(args) == -1)
                goto PRINTERR;
        } else if (strcmp(command, "release") == 0) {
            if(release_command(args) == -1)
                goto PRINTERR;
        } else if (strcmp(command, "wait") == 0) {
            if(wait_command(args) == -1)
                goto PRINTERR;
        } else if (strcmp(command, "kill") == 0) {
            if(kill_command(args) == -1)
                goto PRINTERR;
        } else if (strcmp(command, "peek") == 0) {
            // Read from a process
        } else if (strcmp(command, "poke") == 0) {
            // Write to a process
        } else if (strcmp(command, "bt") == 0) {
            // Show a stack trace
        } else {
            PRINTERR:
            log_error(input_line);
            fprintf(stderr, "?\n");
        }

        free(input_line);
        input_line = NULL;
    }

    return 0;
}
