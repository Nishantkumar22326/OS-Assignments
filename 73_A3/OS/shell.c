// SimpleShell.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>  
#include <sys/resource.h>
#include "SharedMemory.h"

#define MAX_INPUT_SIZE 1024
#define MAX_PROCESSES 100

pid_t sch;

void print_process_info(pid_t pid) {
    struct timeval start_time, end_time;
    struct rusage usage;

    gettimeofday(&end_time, NULL);  // Get the end time
    getrusage(RUSAGE_CHILDREN, &usage);  // Get resource usage of child processes

    if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
        printf("PID: %d\n", pid);
        printf("Execution Time: %ld.%06ld seconds\n",
               usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
        printf("Wait Time: %ld.%06ld seconds\n",
               usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    }
}


void signal_handler(int sig){
    if(sig==SIGALRM){
        kill(sch,SIGCONT);
        pause();
    }
    else if(sig==SIGUSR2){
        kill(sch,SIGSTOP);
        alarm(20);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <number_of_cpus>\n", argv[0]);
        exit(1);
    }
    sch=fork();
    if(sch==0){
        usleep(1000) ;
        execlp("./Scheduler","./Scheduler",NULL);

    }
    else if(sch>0){
        usleep(10000);
        kill(sch,SIGSTOP);
    }
    int num_cpus = atoi(argv[1]);
    int tslice = atoi(argv[2]) ;
    if (num_cpus <= 0 || num_cpus > MAX_PROCESSES)
    {
        printf("Invalid number of CPUs. Must be between 1 and %d.\n", MAX_PROCESSES);
        exit(1);
    }

    // Attach to the shared memory segment
    key_t key = ftok("shmfile", 65);
    int shmid = shmget(key, sizeof(struct SharedMemory), IPC_CREAT | 0666);

    if (shmid == -1)
    {
        perror(" fgh shmget");
        exit(1);
    }

    struct SharedMemory *shared_memory = (struct SharedMemory *)shmat(shmid, NULL, 0);

    if ((void *)shared_memory == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    //signal(SIGUSR1, handle_signal);
    signal(SIGALRM,signal_handler);
    signal(SIGUSR2,signal_handler);
   
    alarm(20);
    while (1)
    {
        shared_memory->ind = 0;
        signal(SIGALRM,signal_handler);
        char input[MAX_INPUT_SIZE];
        printf("SimpleShell$ ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) {
            // Terminate child processes
            for (int i = 0; i < num_cpus; i++) {
                int pid = shared_memory->process_ids[i];
                if (pid > 0) {
                    kill(pid, SIGTERM);  // Send a termination signal
                }
            }
            // Wait for child processes to exit
            for (int i = 0; i < num_cpus; i++) {
                int pid = shared_memory->process_ids[i];
                if (pid > 0) {
                    int status;
                    waitpid(pid, &status, 0);
                    print_process_info(pid); 
                }
            }
            printf("Exiting the shell...\n");
            break;
        }
    


        else if (strncmp(input, "submit", 6) == 0)
        {
            // Extract the program name from the input

            char program_name[MAX_INPUT_SIZE];
            if (sscanf(input, "submit %s", program_name) == 1)
            {
                int pid = fork();
                if (pid == 0)
                {
                    // Child process
                    execl(program_name, program_name, NULL);
                    perror("exec");
                    exit(1);
                }
                else if (pid > 0)
                {
                    usleep(100);
                    kill(pid, SIGSTOP);
                    shared_memory->process_ids[shared_memory->ind] = pid;
                    shared_memory->ind++;   
                    // Parent process
                    // Add the process to the shared memory for scheduling
                }
                else
                {
                    perror("fork");
                }
            }
        }
    }

    return 0;
}