#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "debug.h"
#include "deet.h"
#include "helper.h"

typedef char TFLAG;
#define TRACED 'T'
#define UNTRACED 'U'
typedef void (handler_t)(int);

typedef struct {
    int deetId;
    pid_t pid;
    TFLAG tflag;
    PSTATE state;
    int exit_status;
    char** argv;
} managed_process;

managed_process *process_list[10];

volatile sig_atomic_t child_status_changed = 0;

int quit_command();

managed_process *find_managed_process(pid_t pid){
    for(int i = 0; i < 10; i++){
        if(process_list[i]->pid == pid){
            return process_list[i];
        }
    }
    return NULL;
}

void add_new_process(managed_process *new_process){
    for(int i = 0; i < 10; i++){
        if(process_list[i] == NULL){
            process_list[i] = new_process;
            new_process->deetId = i;
            return;
        }
    }
}

void cleanup_dead_process(int index) {
    if (process_list[index] == NULL) {
        return; // No process to clean up
    }

    // Assuming PSTATE has a state indicating the process is dead
    if (process_list[index]->state == PSTATE_DEAD) {
        // Free dynamically allocated memory
        // free(process_list[index]->exit_status);
        if (process_list[index]->argv != NULL) {
            for (int i = 0; process_list[index]->argv[i] != NULL; i++) {
                free(process_list[index]->argv[i]);
            }
            free(process_list[index]->argv);
        }

        // Free the process structure itself
        free(process_list[index]);

        // Set the pointer to NULL
        process_list[index] = NULL;
    }
}

void cleanup_all_dead_processes() {
    for (int i = 0; i < 10; i++) {
        cleanup_dead_process(i);
    }
}

const char* pstate_to_string(PSTATE state) {
    switch (state) {
        case PSTATE_NONE: return "none";
        case PSTATE_RUNNING: return "running";
        case PSTATE_STOPPING: return "stopping";
        case PSTATE_STOPPED: return "stopped";
        case PSTATE_CONTINUING: return "continuing";
        case PSTATE_KILLED: return "killed";
        case PSTATE_DEAD: return "dead";
        default: return "unknown";
    }
}


void print_managed_process(const managed_process *proc) {
    char buffer[1024];
    int len = 0;

    // Print deetId, pid, and tflag
    if(proc->state == 6){
        len += snprintf(buffer + len, sizeof(buffer) - len, "%d\t%d\t%c\t%s\t0x%x\t", proc->deetId, proc->pid, proc->tflag, pstate_to_string(proc->state), proc->exit_status);
    }else{
        len += snprintf(buffer + len, sizeof(buffer) - len, "%d\t%d\t%c\t%s\t\t", proc->deetId, proc->pid, proc->tflag, pstate_to_string(proc->state));
    }

    // Print argv
    if (proc->argv) {
        for (int i = 0; proc->argv[i] != NULL; i++) {
            len += snprintf(buffer + len, sizeof(buffer) - len, "%s ", proc->argv[i]);
        }
    }

    // Remove the last space and add newline
    if (len > 0 && buffer[len - 1] == ' ') {
        buffer[len - 1] = '\n';
    } else {
        len += snprintf(buffer + len, sizeof(buffer) - len, "\n");
    }

    // Write to standard output
    write(STDOUT_FILENO, buffer, len);
}

int print_all_managed_processes(){
    int flag = 0;
    for(int i = 0; i < 10; i++){
        if(process_list[i] != NULL){
            print_managed_process(process_list[i]);
            flag = 1;
        }
    }
    return flag;
}

void free_managed_process(managed_process *mp) {
    if (mp == NULL) {
        return;
    }

    // Free each string in the argv array, if it's allocated
    if (mp->argv != NULL) {
        for (int i = 0; mp->argv[i] != NULL; ++i) {
            free(mp->argv[i]);
        }
        free(mp->argv); // Free the array itself
    }

    // Finally, free the struct
    free(mp);
}

int is_empty(){
    for(int i = 0; i < 10; i++){
        if(process_list[i] != NULL)
            return 0;
    }
    return 1;
}

void sigchld_handler(int signo) {
    log_signal(signo);
    int status;
    pid_t pid;

    while((pid = waitpid(0, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        // debug("pid: %d\n", pid);
        managed_process *child_process = find_managed_process(pid);

        if (WIFEXITED(status)) {
            log_state_change(child_process->pid, child_process->state, PSTATE_DEAD, WEXITSTATUS(status));
            child_process->state = PSTATE_DEAD;
            child_process->exit_status = WEXITSTATUS(status);
            print_managed_process(child_process);
        }
        if (WIFSIGNALED(status)) {
            log_state_change(child_process->pid, child_process->state, PSTATE_DEAD, WTERMSIG(status));
            child_process->state = PSTATE_DEAD;
            child_process->exit_status = WTERMSIG(status);
            print_managed_process(child_process);
        }
        if (WIFSTOPPED(status)) {
            log_state_change(child_process->pid, child_process->state, PSTATE_STOPPED, WIFSTOPPED(status));
            child_process->state = PSTATE_STOPPED;
            print_managed_process(child_process);
        }
        if (WIFCONTINUED(status)) {
            log_state_change(child_process->pid, child_process->state, PSTATE_RUNNING, WIFCONTINUED(status));
            child_process->state = PSTATE_RUNNING;
            print_managed_process(child_process);
        }
    }
    child_status_changed = 1;
}

void sigint_handler(int sig) {
    log_signal(2);
    quit_command();
    log_shutdown();
    exit(EXIT_SUCCESS);
}

int run_command(const char* program, char* const argv[]) {
    if(argv[0] == NULL)
        return -1;

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    pid_t pid;
    pid = fork();

    if(pid == -1){
        // Handle fork error
        log_error("fork failed");
        log_shutdown();
        exit(EXIT_FAILURE);
    } else if(pid == 0) {
        // Child process
        // Redirect STDOUT to STDERR
        dup2(STDERR_FILENO, STDOUT_FILENO);

        // Enable tracing
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        // debug("h");
        // Execute the programs
        execvp(program, argv);

        // If execvp returns, there was an error
        exit(EXIT_FAILURE);

    } else {
        // Parent process
        managed_process *child_process = malloc(sizeof(managed_process));
        child_process->pid = pid;
        child_process->tflag = TRACED;
        child_process->state = PSTATE_NONE;
        // child_process->exit_status
        child_process->argv = copy_argv(argv);

        cleanup_all_dead_processes();

        add_new_process(child_process);

        log_state_change(pid, child_process->state, PSTATE_RUNNING, 0);
        child_process->state = PSTATE_RUNNING;
        print_managed_process(child_process);

        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        return 0;
    }
}

managed_process* input_validation_and_get_process(char* const argv[]){
    if(is_empty()) return NULL;
    if(argv[0] == NULL) return NULL;
    if(argv[1] != NULL) return NULL;
    int deetId;
    if((deetId = charToInt(*argv[0])) == -1) return NULL;
    return process_list[deetId];
}

int stop_command(char* const argv[]) {
    managed_process *child_process = input_validation_and_get_process(argv);
    if(child_process == NULL) return -1;
    if(child_process->state != PSTATE_RUNNING) return -1;

    if(child_process->tflag == UNTRACED){
        log_state_change(child_process->pid, child_process->state, PSTATE_STOPPING, 0);
        child_process->state = PSTATE_STOPPING;
        print_managed_process(child_process);
    }

    if(kill(child_process->pid, SIGSTOP) == -1) return -1;
    return 0;
}

int kill_command(char* const argv[]) {
    managed_process *child_process = input_validation_and_get_process(argv);
    if(child_process == NULL) return -1;
    if(child_process->state == PSTATE_NONE ||
        child_process->state == PSTATE_KILLED ||
        child_process->state == PSTATE_DEAD)    return -1;
    log_state_change(child_process->pid, child_process->state, PSTATE_KILLED, 0);
    child_process->state = PSTATE_KILLED;
    print_managed_process(child_process);
    if(kill(child_process->pid, SIGKILL) == -1) return -1;
    return 0;
}

int cont_command(char* const argv[]){
    managed_process *child_process = input_validation_and_get_process(argv);
    if(child_process == NULL) {
        return -1;
    }

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    if(child_process->state == PSTATE_STOPPED){
        // Check if the process is being traced
        if(child_process->tflag == TRACED){
            log_state_change(child_process->pid, child_process->state, PSTATE_RUNNING, 0);
            child_process->state = PSTATE_RUNNING;
            print_managed_process(child_process);
            ptrace(PTRACE_CONT, child_process->pid, NULL, NULL);
        }else{
            log_state_change(child_process->pid, child_process->state, PSTATE_CONTINUING, 0);
            child_process->state = PSTATE_CONTINUING;
            print_managed_process(child_process);
            kill(child_process->pid, SIGCONT);
        }
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
    }
    else{
        debug("process state: %s",pstate_to_string(child_process->state));
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        return -1;
    }
    return 0;
}

int release_command(char* const argv[]){
    managed_process *child_process = input_validation_and_get_process(argv);
    if(child_process == NULL) {
        return -1;
    }

    if(child_process->tflag == TRACED && child_process->state != PSTATE_RUNNING){
        ptrace(PTRACE_DETACH, child_process->pid, NULL, NULL);
        log_state_change(child_process->pid, child_process->state, PSTATE_RUNNING, 0);
        child_process->state = PSTATE_RUNNING;
        child_process->tflag = UNTRACED;
        print_managed_process(child_process);
    }else{
        return -1;
    }
    return 0;
}

managed_process* wait_command_input_validation_and_get_process(char* const argv[]){
    if(is_empty()) return NULL;
    if(argv[0] == NULL) return NULL;
    if(argv[1] != NULL){
        if(!((strcmp(argv[1], "running") == 0) ||
            (strcmp(argv[1], "stopping") == 0) ||
            (strcmp(argv[1], "stopped") == 0) ||
            (strcmp(argv[1], "continuing") == 0) ||
            (strcmp(argv[1], "killed") == 0) ||
            (strcmp(argv[1], "dead") == 0))) {
            return NULL;
        }
    }
    if(argv[2] != NULL) return NULL;
    int deetId;
    if((deetId = charToInt(*argv[0])) == -1) return NULL;
    return process_list[deetId];
}

int wait_command(char* const argv[]){
    char* desired_state = "dead";
    managed_process* child_process = wait_command_input_validation_and_get_process(argv);
    if(child_process == NULL)   return -1;
    if(argv[1] != NULL){
        desired_state = argv[1];
    }

    // Block SIGCHLD and save current signal mask
    sigset_t mask, orig_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &orig_mask);
    while(!(strcmp(pstate_to_string(child_process->state), desired_state) == 0)){
        sigsuspend(&orig_mask);
    }
    child_status_changed = 0;
    sigprocmask(SIG_SETMASK, &orig_mask, NULL);
    return 0;
}

int peek_command(const char* argv[]){
    if (argv[0] == NULL || argv[1] == NULL || argv[3] != NULL) {
        return -1;
    }

    // Validate deetId
    int deetId;
    if((deetId = charToInt(*argv[0])) == -1) return -1;

    // Validate Address (hexadecimal)
    char *endptr;
    errno = 0;
    unsigned long long address = strtoull(argv[1], &endptr, 16);
    if (errno != 0 || *endptr != '\0' || address == 0) {
        return -1;
    }

    // Validate Number of Words (if provided)
    int num_words = 1; // Default value
    if (argv[2] != NULL) {
        errno = 0;
        long temp_num_words = strtol(argv[2], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || temp_num_words <= 0) {
            return -1;
        }
        num_words = (int)temp_num_words;
    }

    if(num_words <= 0){
        num_words = 1;
    }

    managed_process* child_process = process_list[(int)deetId];
    if(child_process == NULL)   return -1;
    if(child_process->state != PSTATE_STOPPED)  return -1;
    if(child_process->tflag != TRACED)  return -1;

    for(int i = 0; i < num_words; ++i){
        errno = 0;
        unsigned long long data = ptrace(PTRACE_PEEKDATA, child_process->pid, address + i * 8, NULL);

        if(errno != 0){
            return -1;
        }

        printf("%016llx\t%016llx\n", address + i * 8, data);
    }
    return 0;
}

int poke_command(const char* argv[]){
    // debug("1");
    if (argv[0] == NULL || argv[1] == NULL || argv[2] == NULL || argv[3] != NULL) {
        return -1;
    }
    // debug("2");
    // Validate deetId
    int deetId;
    if((deetId = charToInt(*argv[0])) == -1) return -1;
    // debug("3");

    // Validate Address (hexadecimal)
    char *endptr;
    errno = 0;
    unsigned long long address = strtoull(argv[1], &endptr, 16);
    if (errno != 0 || *endptr != '\0' || address == 0) {
        return -1;
    }
    // debug("4");

    errno = 0;
    unsigned long long data = strtoull(argv[2], &endptr, 16);
    if (errno != 0 || *endptr != '\0' || data == 0) {
        return -1;
    }
    // debug("5");

    managed_process* child_process = process_list[deetId];
    if(child_process == NULL)   return -1;
    if(child_process->state != PSTATE_STOPPED)  return -1;
    if(child_process->tflag != TRACED)  return -1;

    // debug("6");
    errno = 0;
    long ret = ptrace(PTRACE_POKEDATA, child_process->pid, (void*)address, (void*)data);
    // debug("7");
    if(ret == -1 && errno != 0){
        return -1;
    }

    return 0;
}

int bt_command(const char* argv[]){
    if(argv[0] == NULL || argv[2] != NULL){
        return -1;
    }
    long max_length = 10;
    int deetId;
    if((deetId = charToInt(*argv[0])) == -1) return -1;
    managed_process* child_process = process_list[deetId];
    if(child_process == NULL){
        return -1;
    }

    if(argv[1] != NULL){
        char *endptr;
        max_length = strtol(argv[1], &endptr, 10);
        if(endptr == argv[1]){
            return -1;
        }
    }

    struct user_regs_struct regs;
    long stack_pointer, return_address;
    int frame_count = 0;

    if(ptrace(PTRACE_GETREGS, child_process->pid, NULL, &regs) == -1) {
        perror("ptrace GETREGS");
        ptrace(PTRACE_DETACH, child_process->pid, NULL, NULL);
        return -1;
    }

    stack_pointer = regs.rbp; // Base pointer value

    // Traversing the stack frames
    while (frame_count < max_length) {
        // Read the return address from the stack
        return_address = ptrace(PTRACE_PEEKDATA, child_process->pid, stack_pointer + 8, NULL);
        if (errno != 0) {
            break;
        }

        // Print stack frame address and return address
        printf("%016lx\t%016lx\n", stack_pointer, return_address);
        frame_count++;

        // Read the next stack pointer
        stack_pointer = ptrace(PTRACE_PEEKDATA, child_process->pid, stack_pointer, NULL);
        if (errno != 0 || stack_pointer == 0) {
            break;
        }
    }

    return 0;
}

int quit_command() {
    for (int i = 0; i < 10; i++) {
        if (process_list[i] != NULL && (process_list[i]->state != PSTATE_DEAD && process_list[i]->state != PSTATE_KILLED)){
            log_state_change(process_list[i]->pid, process_list[i]->state, PSTATE_KILLED, 0);
            process_list[i]->state = PSTATE_KILLED;
            print_managed_process(process_list[i]);
            if (kill(process_list[i]->pid, SIGKILL) == -1) {
                return -1;
            }
        }
    }

    for(int i = 0; i < 10; i++){
        if(process_list[i] != NULL && process_list[i]->state != PSTATE_DEAD){
            while(1){
                usleep(1);
                if(process_list[i]->state == PSTATE_DEAD)   break;
            }
        }
    }

    for(int i = 0; i < 10; i++){
        if(process_list[i] != NULL){
            free_managed_process(process_list[i]);
        }
    }

    return 0;
}


