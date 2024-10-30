#include "bash.h"

struct history cmd_history[MAX_HISTORY_SIZE];                                                 // array to store command history
static int history_count = 0;                                                                 // counter for the number of stored commands
int is_bg_process = 0;                                                                        // flag to indicate if a process is a background process
int bg_process_count = 0;                                                                    // counter for the number of background process
clock_t start_time,end_time;                                                                 // variable to track start and end time of process
struct BackgroundProcess background_running_processes[MAX_BACKGROUND_PROCESSES];              // array to store running background process


// code of assignments 2 starts here 
int command_history_storeing(char **cmd, pid_t pid, double difference) {
    struct history *entry = &cmd_history[history_count];
    history_count++;
    entry->sno = history_count;
    entry->cmd = (char **)malloc((MAX_TOKENS + 1) * sizeof(char *));
    
    if (entry->cmd == NULL) {
        fprintf(stderr, "Memory allocation error for history command.\n");
        exit(FAILED);
    }

    int i = 0;
    while (cmd[i] != NULL) {
        entry->cmd[i] = strdup(cmd[i]);
        if (entry->cmd[i] == NULL) {
            fprintf(stderr, "Memory allocation error for history command part.\n");
            exit(FAILED);
        }
        i++;
    }
    
    entry->cmd[i] = NULL;
    entry->pid = pid;
    entry->execution_time = difference;
    entry->time = time(NULL);

    return 0;
}


char **slice_command(char **cmd, int start_time, int end_time) {
    if (cmd == NULL || cmd[start_time] == NULL || start_time > end_time) {
        return NULL;
    }

    int num_tokens = end_time - start_time + 1;
    char **sliced_cmd = (char **)malloc((num_tokens + 1) * sizeof(char *)); // +1 for the NULL terminator

    if (sliced_cmd == NULL) {
        fprintf(stderr, "Memory allocation error for sliced command.\n");
        exit(FAILED);
    }

    int i, j = 0;
    for (i = start_time; i <= end_time; i++) {
        sliced_cmd[j] = strdup(cmd[i]);
        if (sliced_cmd[j] == NULL) {
            fprintf(stderr, "Memory allocation error for sliced command part.\n");
            exit(FAILED);
        }
        j++;
    }

    sliced_cmd[j] = NULL; // Null-terminate the array

    return sliced_cmd;
}

void execute_piped_commands(char **cmd) {
    int cmd_count = 0;
    int pipe_count = 0;
    int i = 0;

    // Count the number of pipes to determine the number of commands
    while (cmd[i] != NULL) {
        if (strcmp(cmd[i], "|") == 0) {
            pipe_count++;
        } else if (strcmp(cmd[i], "&") == 0) {
            is_bg_process = 1; // Set the background flag
            cmd[i] = NULL;
        }
        cmd_count++;
        i++;
    }

    if (pipe_count == 0) {
        // No pipes, execute a single command
        exec_cmd(cmd);
        return;
    }

    // Allocate memory for an array of command arrays
    char ***commands = (char ***)malloc((pipe_count + 1) * sizeof(char **));

    if (commands == NULL) {
        fprintf(stderr, "Memory allocation error for commands.\n");
        exit(FAILED);
    }

    int cmd_start_time = 0;
    int cmd_end_time = 0;
    i = 0;

    // Split the input command into multiple commands
    while (i < cmd_count) {
        if (strcmp(cmd[i], "|") == 0) {
            commands[cmd_end_time] = slice_command(cmd, cmd_start_time, i - 1);
            cmd_start_time = i + 1;
            cmd_end_time++;
        }
        i++;
    }

    // Handle the last command
    commands[cmd_end_time] = slice_command(cmd, cmd_start_time, i - 1);

    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;

    // Execute the commands in a pipeline
    for (i = 0; i <= pipe_count; i++) {
        int pipefd[2];

        if (i < pipe_count) {
            // Create a pipe for commands except the last one
            if (pipe(pipefd) == -1) {
                perror("Pipe creation failed");
                exit(FAILED);
            }
            output_fd = pipefd[1]; // Set output to the write end_time of the pipe
        } else {
            // Last command, no need to create a pipe
            output_fd = STDOUT_FILENO;
        }

        // Execute the current command
        new_process(commands[i], input_fd, output_fd);

        // Close input_fd if it's not the initial STDIN_FILENO
        if (input_fd != STDIN_FILENO) {
            close(input_fd);
        }

        // Close output_fd if it's not the last command's STDOUT_FILENO
        if (output_fd != STDOUT_FILENO) {
            close(output_fd);
        }

        // Set the input_fd for the next command to be the read end_time of the pipe
        input_fd = pipefd[0];
    }

    // Wait for all child processes to complete
    int status;
    for (i = 0; i <= pipe_count; i++) {
        wait(&status);
    }

    if (!is_bg_process) {
        int status;
        for (i = 0; i <= pipe_count; i++) {
            wait(&status);
        }
    }

    // Free memory allocated for command arrays
    for (i = 0; i <= pipe_count; i++) {
        free(commands[i]);
    }
    free(commands);
}

int exec_cmd(char **cmd) {
    char *funclist[] = {
        "cd",
        "exit",
        "help",
        "jobs",
        "history"
    };

    int (*func_list[])(char **) = {
        &cd,
        &exiting,
        &help,
        &print_jobs,
        &print_history
    };

    if (cmd[0] == NULL) {
        return -1;
    }

    for (long unsigned int i = 0; i < sizeof(funclist) / sizeof(char *); i++) {
        if (strcmp(cmd[0], funclist[i]) == 0) {
            // printf("%s\n",cmd[0]);
            return ((*func_list[i])(cmd));
        }
    }

    return new_process(cmd, STDIN_FILENO, STDOUT_FILENO);
}


int new_process(char **cmd, int input_fd, int output_fd) {
    pid_t process_id;
    int stat;
    start_time = clock();

    process_id = fork();

    if (process_id == 0) {
        
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        if (output_fd != STDOUT_FILENO){
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Handle output redirection when '>' is detected
        int output_redirect = 0;
        for (int i = 0; cmd[i] != NULL; i++) {
            if (strcmp(cmd[i], ">") == 0) {
                output_redirect = 1;
                cmd[i] = NULL; // Remove the '>' symbol
                i++; // Move to the next argument which should be the output file
                if (cmd[i] != NULL) {
                    int fd = open(cmd[i], O_WRONLY | O_TRUNC);
                    if (fd == -1) {
                        perror("open");
                        exit(FAILED); 
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                } else {
                    fprintf(stderr, "Missing output file after '>'\n");
                    exit(FAILED);
                }
            }
        }

        if (output_redirect) {
            // If output redirection is used, execute the command and exit
            if (execvp(cmd[0], cmd) == -1) {
                fprintf(stderr, "Error in child process.\n");
                exit(FAILED);
            }
        } else {
            // If no output redirection, check for background processing
            if (is_bg_process) {
                // If it's a background process, redirect output to /dev/null
                int null_fd = open("home/assignment 2", O_WRONLY | O_TRUNC);
                if (null_fd == -1) {
                    perror("open");
                    exit(FAILED);
                }
                dup2(null_fd, STDOUT_FILENO);
                close(null_fd);
            }

            // Execute the command as a foreground or background process
            if (execvp(cmd[0], cmd) == -1) {
                fprintf(stderr, "Error in child process.\n");
                exit(FAILED);
            }
        }
    } else if (process_id < 0) {
        fprintf(stderr, "Error while forking.\n");
    } else {
        // Parent process
        if (!is_bg_process) {
            // Wait for the child process to complete if not in the background
            waitpid(process_id, &stat, 0);
            end_time = clock();
            double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            command_history_storeing(cmd, process_id, elapsed_time);
        } else {
            // In the background, fork again to detach from the terminal
            pid_t bg_process_id = fork();
            if (bg_process_id == 0) {
                // Child of the parent process (detached background process)
                // This child will immediately exit, leaving the background process running
                exit(0);
            } else if (bg_process_id < 0) {
                fprintf(stderr, "Error while forking background process.\n");
            } else {
                // Parent of the detached background process
                // Add the process to the list
                if (bg_process_count < MAX_BACKGROUND_PROCESSES) {
                    background_running_processes[bg_process_count].pid = process_id;
                    strncpy(background_running_processes[bg_process_count].cmd, cmd[0], sizeof(background_running_processes[bg_process_count].cmd));
                    end_time = clock();
                    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
                    command_history_storeing(cmd, bg_process_id, elapsed_time);
                    bg_process_count++;
                } else {
                    fprintf(stderr, "Maximum number of background processes reached.\n");
                }
                is_bg_process = 0;
            }
        }
    }

    return -1;
}

int help(char **cmd){
    char *funclist[] = {
        "cd",
        "exit",
        "help",
        "jobs",
        "history"
    };

    (void)(**cmd);

    printf("This is a simple shell created by Harsh and Aditya.\n");
    printf("The commands that are implemented are:- \n");
    for (int i = 0; i < 3; i++){
        printf("\t-> %s\n", funclist[i]);
    }
    printf("Reason for the implementation of these commands are available in the documentation.\n");
    end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    command_history_storeing(cmd, 0, elapsed_time);
    return 0;
}

int exiting(char **cmd){
    if (cmd[1]) return (atoi(cmd[1]));
    else{
        if (cmd_history[0].cmd == NULL){
            printf("\nNo Commands Executed\n");
            exit(SUCCESS);
        }
        printf("\n\nHistory:\n");
        for (int i = 0; i < history_count; i++){
            printf("[%d]\nPID: %d\n", cmd_history[i].sno, cmd_history[i].pid);
            printf("Command Executed: ");
            int j = 0;
            while (cmd_history[i].cmd[j] != NULL){
                printf("%s ", cmd_history[i].cmd[j]);
                j++;
            }
            printf("\nTimeStamp at which command was executed: %sExecution Time: %f\n", ctime(&cmd_history[i].time) + 11, cmd_history[i].execution_time);
            printf("\n");
        }
        exit(0);
    }
    return 0;
}

int cd(char **cmd){
    if(cmd[1] != NULL){
        if(chdir(cmd[1]) != 0) {printf("error in cd.c: changing dir\n"); return 1;}
        end_time = clock();
        double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        command_history_storeing(cmd, 0, elapsed_time);
    }else fprintf(stderr,"expected argument to \"cd\"\n");
    return (-1);
}

int print_jobs() {
    if (bg_process_count == 0){
        printf("NO CURRENT PROCESS RUNNING IN THE BACKGROUND\n");
    } else {
        printf("Background processes:\n");
        for (int i = 0; i < bg_process_count; i++) {
            printf("[%d] %s (PID: %d)\n", i + 1, background_running_processes[i].cmd, background_running_processes[i].pid);
        }
    }
    return 0;
}

int print_history(){
    printf("History:\n");
    for (int i = 0; i < history_count; i++){
        printf("[%d]\nPID: %d\n", cmd_history[i].sno, cmd_history[i].pid);
        printf("Command Executed: ");
        int j = 0;
        while (cmd_history[i].cmd[j] != NULL){
            printf("%s ", cmd_history[i].cmd[j]);
            j++;
        }
        printf("\n");
    }
    return 0;
}

char *read_line(void) {
    char *line = NULL;
    size_t buffersize = 0;

    ssize_t read = getline(&line, &buffersize, stdin);

    if (read == -1) {
        if (feof(stdin)) {
            free(line);
            return NULL;
        } else {
            free(line);
            perror("Line not read properly.\n");
            exit(FAILED);
        }
    }
    return line;
}

char **parse(char *line) {
    int buffersize = 64;
    int token_counter = 0;
    char **tokens = malloc(buffersize * sizeof(char *));

    if (tokens == NULL) {
        fprintf(stderr, "Memory allocation error for tokens.\n");
        exit(FAILED);
    }

    if (line == NULL) {
        fprintf(stderr, "Input line is NULL.\n");
        exit(FAILED);
    }

    char *line_copy = strdup(line);

    if (line_copy == NULL) {
        fprintf(stderr, "Memory allocation error for line_copy.\n");
        exit(FAILED);
    }

    char *token = strtok(line_copy, DELIMITER); // Tokenize by space, tab, and newline

    while (token != NULL) {
        if (strcmp(token, "#") == 0) {
            break;
        }

        if (strcmp(token, "|") == 0) {
            tokens[token_counter] = strdup(token);
            token_counter++;
        } else {
            tokens[token_counter] = strdup(token);
            token_counter++;
        }

        if (token_counter >= buffersize) {
            buffersize *= 2;
            tokens = realloc(tokens, buffersize * sizeof(char *));

            if (tokens == NULL) {
                fprintf(stderr, "Memory reallocation error.\n");
                exit(FAILED);
            }
        }

        token = strtok(NULL, " \t\n");
    }

    tokens[token_counter] = NULL;


    free(line_copy);
    return tokens;
}

void sigquit_handler(int sig){
    if (sig == SIGQUIT){
        printf("SIGQUIT signal received\n");
        exit(SUCCESS);
    }
    printf("\n\nHistory:\n");
    for (int i = 0; i < history_count; i++){
        printf("[%d]\nPID: %d\n", cmd_history[i].sno, cmd_history[i].pid);
        printf("Command Executed: ");
        int j = 0;
        while (cmd_history[i].cmd[j] != NULL){
            printf("%s ", cmd_history[i].cmd[j]);
            j++;
        }
        printf("\nTimeStamp at which command was executed: %sExecution Time: %f\n", ctime(&cmd_history[i].time) + 11, cmd_history[i].execution_time);
        printf("\n");
    }
}

void sigint_handler(int sig) {
    if (sig == SIGINT) {
        if (cmd_history[0].cmd == NULL){
            printf("\nNo Commands Executed\n");
            exit(SUCCESS);
        }
        printf("\n\nHistory:\n");
        for (int i = 0; i < history_count; i++){
            printf("[%d]\nPID: %d\n", cmd_history[i].sno, cmd_history[i].pid);
            printf("Command Executed: ");
            int j = 0;
            while (cmd_history[i].cmd[j] != NULL){
                printf("%s ", cmd_history[i].cmd[j]);
                j++;
            }
            printf("\nTimeStamp at which command was executed: %sExecution Time: %f\n", ctime(&cmd_history[i].time) + 11, cmd_history[i].execution_time);
            printf("\n");
        }
    }
    exit(SUCCESS);
}
// code of assignments 2 ends here 

int main(int argc, char **argv){
    int status = -1;
    char *line;
    char cwd[1024];

    while (status == -1) { 
        printf("himanshu@Dam_Shell:%s$ ", getcwd(cwd, sizeof(cwd)));
        if (signal(SIGINT, sigint_handler) == SIG_ERR){
            fprintf(stderr, "Error while getting signal\n");
            exit(FAILED);
        }
        line = read_line();
        char **cmd = parse(line);
        if (strcmp(cmd[0], "submit") == 0)
        {
            process_submit(cmd);
        }
        else if (strcmp(cmd[0], "sched") == 0){
            pid_t sched_pid = fork();
            if (sched_pid == 0)
            {
                // This is the child process
                // Fork again to create a grandchild process
                pid_t gchild_pid = fork();
                if (gchild_pid == 0) {
                    // This is the grandchild process
                    dummy_main(argc, argv);
                    exit(0); // Terminate the grandchild process
                } else if (gchild_pid < 0) {
                    fprintf(stderr, "Error in forking grandchild process\n");
                } else {
                    // The child process (scheduler) doesn't need to wait for the grandchild
                    exit(0);
                }
                dummy_main(argc, argv);
                exit(0);
            } 
            else if (sched_pid < 0) {
                fprintf(stderr, "Error in forking scheduler process\n");
            } 
            else {
                // This is the parent process (shell)
                // Wait for the scheduler to finishclear
                int sched_status;
                waitpid(sched_pid, &sched_status, 0);
                 if (WIFEXITED(sched_status)) {
                    printf("Scheduler process has completed with exit status: %d\n", WEXITSTATUS(sched_status));
                } else {
                    printf("Scheduler process terminated abnormally\n");
                }
            }
        }
        else if (cmd != NULL) {
            execute_piped_commands(cmd);
            free(cmd);
        }
        if (line != NULL) {free(line);}
    }
}


