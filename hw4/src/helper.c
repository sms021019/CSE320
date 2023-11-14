#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


#include "deet.h"

char* get_first_argument(const char* input_line) {
    // Make a copy of the input_line because strtok modifies the string.
    char* line_copy = strdup(input_line);
    if (line_copy == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    // Use strtok to find the first token, which is the first argument.
    char* first_arg = strtok(line_copy, " \t\n");
    if (first_arg != NULL) {
        // Make a copy of the first argument because line_copy will be modified further.
        first_arg = strdup(first_arg);
    }

    // Clean up the line copy.
    free(line_copy);

    return first_arg; // Caller is responsible for freeing this memory.
}

int count_arguments(const char *input_line) {
    // Make a copy of the input_line because strtok modifies the string.
    char *line_copy = strdup(input_line);
    if (line_copy == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return -1; // Return -1 to indicate an error.
    }

    int count = 0;
    // Use strtok to tokenize the string by spaces, tabs, and newlines.
    char *token = strtok(line_copy, " \t\n");
    while (token != NULL) {
        count++;
        token = strtok(NULL, " \t\n"); // Continue tokenizing the string.
    }

    free(line_copy); // Clean up the line copy.
    return count;
}

void print_help() {
    printf("Available commands:\n"
           "help -- Print this help message\n"
           "quit -- Quit the program\n"
           "show -- Show process info\n"
           "run -- Start a process\n"
           "stop -- Stop a running process\n"
           "cont -- Continue a stopped process\n"
           "release -- Stop tracing a process, allowing it to continue normally\n"
           "wait -- Wait for a process to enter a specified state\n"
           "kill -- Forcibly terminate a process\n"
           "peek -- Read from the address space of a traced process\n"
           "poke -- Write to the address space of a traced process\n"
           "bt -- Show a stack trace for a traced process\n");
}

char** copy_argv(char* const argv[]) {
    int argc;
    for (argc = 0; argv[argc] != NULL; argc++); // Count arguments

    // Allocate memory for the array of pointers
    char** new_argv = malloc((argc + 1) * sizeof(char*));
    if (new_argv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Copy each argument
    for (int i = 0; i < argc; i++) {
        new_argv[i] = strdup(argv[i]);
        if (new_argv[i] == NULL) {
            perror("strdup");
            // Free already allocated memory
            while (i > 0) {
                free(new_argv[--i]);
            }
            free(new_argv);
            exit(EXIT_FAILURE);
        }
    }

    // NULL-terminate the array
    new_argv[argc] = NULL;

    return new_argv;
}

int charToInt(char c) {
    // Check if c is between '0' (48 in ASCII) and '9' (57 in ASCII)
    if (c >= '0' && c <= '9') {
        return c - '0'; // Subtract ASCII value of '0' to get the integer value
    } else {
        return -1; // Return -1 or some error code if c is not a numeric character
    }
}
