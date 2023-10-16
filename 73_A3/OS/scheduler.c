// SimpleScheduler.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include "SharedMemory.h" // Include the SharedMemory header
#include <semaphore.h>

#define MAX_PROCESSES 100
#define QUANTUM_MS 1000 // Time quantum in milliseconds
sem_t mutex ;

int process_count = 0;
int current_process = 0;
int num_cpus;
struct SharedMemory *shared_memory;
pid_t parent;
struct Queue
{
    int processes[MAX_PROCESSES];
    int front, rear, size;
};

struct Queue ready_queue;

void run_processes();
void initialize_queue()
{
    ready_queue.front = ready_queue.rear = -1;
    ready_queue.size = 0;
}

bool is_empty()
{
    return ready_queue.size == 0;
}

bool is_full()
{
    return ready_queue.size == MAX_PROCESSES;
}

void enqueue(int process_id)
{
    if (is_full())
    {
        printf("Queue is full. Cannot enqueue.\n");
        return;
    }
    if (is_empty())
    {
        ready_queue.front = ready_queue.rear = 0;
    }
    else
    {
        ready_queue.rear = (ready_queue.rear + 1) % MAX_PROCESSES;
    }
    ready_queue.processes[ready_queue.rear] = process_id;
    ready_queue.size++;
}

int dequeue()
{
    if (is_empty())
    {
        printf("Queue is empty. Cannot dequeue.\n");
        return -1;
    }
    int process_id = ready_queue.processes[ready_queue.front];
    if (ready_queue.front == ready_queue.rear)
    {
        ready_queue.front = ready_queue.rear = -1;
    }
    else
    {
        ready_queue.front = (ready_queue.front + 1) % MAX_PROCESSES;
    }
    ready_queue.size--;
    return process_id;
}

void handle_signal(int signum) {
    if (signum == SIGUSR1) {
        // Signal handler: enqueue the signaled process
        int process_id = shared_memory->process_ids[shared_memory -> ind];
        if (process_id > 0) {
            enqueue(process_id);
        }
    }
}

int main(int argc, char *argv[])
{  
    printf("hi\n");
    if (argc != 2)
    {
        printf("Usage: %s <number_of_cpus>\n", argv[0]);
        exit(1);
    }
    parent=getppid();
    num_cpus = atoi(argv[1]);
    if (num_cpus <= 0 || num_cpus > MAX_PROCESSES)
    {
        printf("Invalid number of CPUs. Must be between 1 and %d.\n", MAX_PROCESSES);
        exit(1);
    }

    key_t key = ftok("shmfile", 65);    
    int shmid = shmget(key, sizeof(struct SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }

    shared_memory = (struct SharedMemory *)shmat(shmid, NULL, 0);
    if ((void *)shared_memory == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }


    signal(SIGUSR1, handle_signal);
    signal(SIGCONT, handle_signal);
    initialize_queue();
    printf("SimpleScheduler started with %d CPU(s).\n", num_cpus);
    while (1)
    {
        run_processes();
    }

    return 0;
}
void run_processes() {
    while (!is_empty()) {
        int remaining_cpus = num_cpus;
        while (remaining_cpus > 0) {
            int process_id = dequeue();
            if (process_id > 0) {
                pid_t child_pid = fork();

                if (child_pid == -1) {
                    perror("fork");
                    exit(1);
                } else if (child_pid == 0) {
                    // Child process
                    sem_wait(&mutex); // Acquire the semaphore
                    kill(process_id, SIGCONT); // Resume the process
                    sem_post(&mutex); // Release the semaphore

                    int st;
                    int res = waitpid(process_id, &st, 0);
                    if (res == process_id) {
                        if (WIFEXITED(st)) {
                            printf("Process with PID %d completed.\n", process_id);
                            fflush(stdout) ;
                        }
                    }
                    printf("Paused process with PID: %d\n", process_id);
                    fflush(stdout) ;
                    exit(0);
                } else {
                    // Parent process
                    remaining_cpus--;
                }
            }
        }

        // Wait for child processes to finish
        while (remaining_cpus > 0) {
            int st;
            wait(&st);
            remaining_cpus--;
        }
    }
    kill(parent,SIGUSR2);
}

