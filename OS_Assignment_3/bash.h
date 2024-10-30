#ifndef BASH_H
#define BASH_H
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/ptrace.h>


                                                    // code snippet from assignment 2 //

// here we define some 
#define DELIMITER " \t\r\n\a\""
#define SUCCESS 0
#define FAILED 1
#define MAX_BACKGROUND_PROCESSES 100
#define MAX_TOKENS 64
#define TIME_QUANTUM 4
#define MAX_HISTORY_SIZE 1024

// creat struct about history things
struct history{
    int sno;
    pid_t pid;
    char **cmd;
    time_t time;
    double execution_time;
};

// struct for handling background command
struct BackgroundProcess{
    int sno;
    pid_t pid;
    char cmd[MAX_TOKENS];
};

// functions required by simpleshell
char *read_line(void); 
char **parse(char *line);
int exec_cmd(char **cmd);
void execute_piped_commands(char **cmd);

// functions required for execute the command
int new_process(char **cmd, int input_fd, int output_fd);
char** slice_command(char **cmd, int start, int end);
int command_history_storeing(char **cmd, pid_t pid, double difference);
int cd(char **cmd);

// extra function implementd for more commands
int help(char **cmd); 
int exiting(char **cmd);
int print_jobs();
int print_history();
int main(int argc, char **argv);

                                              // code snippet from assignment 3 //

// declare array to store processes and command length
#define MAX_PROCESSES 100
#define MAX_COMMAND_LENGTH 100

// define struct about timings of process
struct process {
    pid_t pid;
    time_t arrival_time;
    time_t start_time;
    time_t end_time;
    time_t wait_time;
    time_t burst_time;
};

// define struct about queue 
struct queue {
    struct process processes[MAX_PROCESSES];
    int front;
    int rear;
    int size;
};

// function for schedule the processes
int is_queue_empty(struct queue *proc_queue);
void add_to_queue(struct queue *proc_queue, struct process new_process);
struct process remove_from_queue(struct queue *proc_queue);
void process_scheduler();
void process_submit(char **cmd);
void handle_finished_processes();
int dummy_main(int argc, char **argv);
#endif