int run_command(const char* program, char* const argv[]);
int stop_command(char* const argv[]);
int kill_command(char* const argv[]);
int cont_command(char* const argv[]);
int wait_command(char* const argv[]);
int peek_command(char* const argv[]);
int poke_command(char* const argv[]);
int bt_command(char* const argv[]);
int quit_command();
int release_command(char* const argv[]);
int print_all_managed_processes();
void sigchld_handler(int signo);
void sigint_handler(int sig);


