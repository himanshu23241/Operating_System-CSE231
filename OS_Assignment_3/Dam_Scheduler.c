#include "bash.h"

// Initialize the ready_run_queue
struct queue ready_run_queue = { .front = 0, .rear = -1, .size = 0 };

// Function to add a process to the queue
void add_to_queue(struct queue *proc_queue, struct process new_process) {
    if (proc_queue->size == MAX_PROCESSES) {
        printf("Queue is full\n");
        return;
    }
    proc_queue->rear = (proc_queue->rear + 1) % MAX_PROCESSES;
    proc_queue->processes[proc_queue->rear] = new_process;
    proc_queue->size++;
}

// Function to remove a process from the queue
struct process remove_from_queue(struct queue *proc_queue) {
    if (proc_queue->size == 0) {
        printf("Queue is empty\n");
        struct process blank_process = {-1, 0, 0, 0, 0, 0};
        return blank_process;
    }
    struct process new_process = proc_queue->processes[proc_queue->front];
    proc_queue->front = (proc_queue->front + 1) % MAX_PROCESSES;
    proc_queue->size--;
    return new_process;
}

// Function to check if the queue is empty
int is_queue_empty(struct queue *proc_queue) {
    return proc_queue->size == 0;
}

sem_t *mutexLock;
sem_t *itemsFull;
sem_t *itemsEmpty;

int ncpu;
int tslice;

// Start processes
void start_processes() {
    for (int i = 0; i < ncpu && !is_queue_empty(&ready_run_queue); i++) {
        struct process new_process = remove_from_queue(&ready_run_queue);
        new_process.start_time = time(NULL);
        kill(new_process.pid, SIGCONT);
        printf("Process %d started\n", new_process.pid);
    }
}

// Add process to queue
void process_submit(char **command) {
    pid_t pid = fork();
    if (pid == 0) {
        kill(getpid(), SIGSTOP);
        execvp(command[1], command + 1);
        perror("execvp failed");
        exit(1);
    } else if (pid < 0) {
        printf("Error in creating process\n");
    } else {
        struct process new_process = {pid, time(NULL), 0, 0, 0, 0};
        add_to_queue(&ready_run_queue, new_process);
        printf("Process %d added to queue\n", pid);
    }
}

// Handle finished processes
void handle_finished_processes() {
    for (int i = 0; i < ncpu; i++) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            printf("Process %d finished, checking queue\n", pid);
            for (int j = 0; j < ready_run_queue.size; j++) {
                struct process *proc = &ready_run_queue.processes[(ready_run_queue.front + j) % MAX_PROCESSES];
                if (proc->pid == pid) {
                    proc->end_time = time(NULL);
                    proc->burst_time = proc->end_time - proc->start_time;
                    proc->wait_time = proc->start_time - proc->arrival_time - proc->burst_time;
                    printf("Process %d: pid=%d, execution time=%ld, wait time=%ld\n", j + 1, proc->pid, proc->burst_time, proc->wait_time);
                    remove_from_queue(&ready_run_queue);
                    break;
                }
            }
        }
    }
}

// Process scheduler
void process_scheduler() {
    while (!is_queue_empty(&ready_run_queue)) {
        sem_wait(itemsFull);
        sem_wait(mutexLock);

        start_processes();

        sem_post(mutexLock);
        usleep(tslice * 1000);

        sem_wait(mutexLock);
        handle_finished_processes();
        sem_post(itemsEmpty);
        sem_post(mutexLock);
    }
}

// Main function for the scheduler
int dummy_main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <NCPU> <TSLICE>\n", argv[0]);
        return 1;
    }
    ncpu = atoi(argv[1]);
    tslice = atoi(argv[2]);
    if (ncpu <= 0 || tslice <= 0) {
        printf("Invalid input\n");
        return 1;
    }

    mutexLock = sem_open("mutex", O_CREAT, 0666, 1);
    itemsFull = sem_open("full", O_CREAT, 0666, 0);
    itemsEmpty = sem_open("empty", O_CREAT, 0666, MAX_PROCESSES);

    if (mutexLock == SEM_FAILED || itemsFull == SEM_FAILED || itemsEmpty == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    sem_wait(itemsEmpty);
    sem_post(itemsFull);

    while (1) {
        process_scheduler();
    }

    sem_close(mutexLock);
    sem_close(itemsFull);
    sem_close(itemsEmpty);
    sem_unlink("mutex");
    sem_unlink("full");
    sem_unlink("empty");
    return 0;
}
