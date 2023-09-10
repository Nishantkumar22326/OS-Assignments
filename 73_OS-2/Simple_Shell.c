//IMPORTING ALL THE NECESSARY LIBRARIES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

#define MAX_INPUT_SIZE 1024
#define MAX_HISTORY_SIZE 100

// Structure to store  history of commands
typedef struct {
    char command[MAX_INPUT_SIZE];
    pid_t pid;
    time_t startingTime;
    double time_span;
    double memory_used;
    int historyCount;

} CommandHistory;

CommandHistory history[MAX_HISTORY_SIZE];
int historyCount = 0; // counting of number of commands


// Function to store history of commands in the Command history struct
void addToHistory(CommandHistory history[], char *command, pid_t pid, time_t startingTime, double time_span, double memory_used, int historyCount) {
    //ERROR HANDLING
    if (historyCount < MAX_HISTORY_SIZE) {  // CHECKING IF THE HISTORY COUNT IS LESS THAN MAX. HISTORY SIZE (ERROR HANDLING)
        strcpy(history[historyCount].command, command);
        history[historyCount].pid = pid;
        history[historyCount].startingTime = startingTime;
        history[historyCount].time_span = time_span;
        history[historyCount].memory_used = memory_used;
        historyCount++;
    } else {
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
            strcpy(history[i].command, history[i + 1].command);
            history[i].pid = history[i + 1].pid;
            history[i].startingTime = history[i + 1].startingTime;
            history[i].time_span = history[i + 1].time_span;
            history[i].memory_used = history[i + 1].memory_used;
        }
        strcpy(history[MAX_HISTORY_SIZE - 1].command, command);
        history[MAX_HISTORY_SIZE - 1].pid = pid;
        history[MAX_HISTORY_SIZE - 1].startingTime = startingTime;
        history[MAX_HISTORY_SIZE - 1].time_span = time_span;
        history[MAX_HISTORY_SIZE - 1].memory_used = memory_used;
    }
}

// Function to show the history of the commands
void Show_History(CommandHistory history[], int historyCount) {
    for (int i = 0; i < historyCount; i++) {
        printf("COMMAND: ");
        printf("%s\n", history[i].command);
        printf("PROCESS ID: ");
        printf("%d\n", (int)history[i].pid);
        printf("PROCESS STARTING TIME: ");
        time_t start_time = history[i].startingTime;
        struct tm *timeinfo = localtime(&start_time);
        char time_str[1000];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        printf("%s\n", time_str);
        printf("TIME SPAN: ");
        printf("%0.5lf\n", history[i].time_span);
        printf("MEMORY USED IN kb: ");
        printf("%2lf\n", history[i].memory_used);
        printf("------------------------------\n");
    }
}

/*Custom handler method that would get invoked when the assigned */
static void handler(int signum) {
    static int count = 0;
    if (signum == SIGINT) {
        char arr[23] = "\ncaught SIGINT signal\n"; 
        write(STDOUT_FILENO, arr, 23);
        printf("\nTERMINATING THE PROCESS \n");
        printf("COMMAND HISTORY: \n");
        Show_History(history, historyCount);
        if (count++ == 1) {
            char arr2[21] = "\nCannot handle more\n"; 
            write(STDOUT_FILENO, arr2, 21);
            exit(0);
        }
        exit(EXIT_SUCCESS); //Exit as victors
    } else if (signum == SIGCHLD) {
        char arr[24] = "Caught SIGCHLD signal\n";
        write(STDOUT_FILENO, arr, 24);
    }
}

void command_prompt() {
    printf("SIMPLE SHELL"); // your old friendlyhood command prompt 
}

// FUNCTION TO CREATE THE PROCESS
int create_process(char *input) {
    char *command[MAX_INPUT_SIZE];
    int status = fork();
    // ERROR HANDLING
    if (status < 0) {
        printf("SOMETHING WENT WRONG!!!!\n"); // THROWING ERROR 
    } else if (status == 0) {
        int count = 0;
        char *token = strtok(input, " ");
        while (token != NULL) {
            command[count++] = token;
            token = strtok(NULL, " \n");
        }
        command[count] = NULL;

        int num_pipes = 0;
        int pipe_pos[MAX_INPUT_SIZE];

        // Find if there are any pipes in the command 
        for (int i = 0; i < count; i++) {
            if (strcmp(command[i], "|") == 0) {
                pipe_pos[num_pipes++] = i;
            }
        }

        int pipe_fd[2];
        int prev_pipe = 0;

        for (int i = 0; i <= num_pipes; i++) {
            if (i < num_pipes) {
                if (pipe(pipe_fd) == -1) {
                    perror("Pipe creation failed");
                    exit(EXIT_FAILURE);
                }
            }

            int child_pid = fork();

            if (child_pid == -1) {
                perror("Fork failed");
                exit(EXIT_FAILURE);
            }

            if (child_pid == 0) {
                // Child process
                if (i > 0) {
                    // Set the input to come from the previous pipe
                    dup2(prev_pipe, STDIN_FILENO);
                    close(prev_pipe);
                }

                if (i < num_pipes) {
                    // Set the output to go to the current pipe
                    dup2(pipe_fd[1], STDOUT_FILENO);
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);
                }

                // Execute the command
                command[pipe_pos[i]] = NULL;
                if (execvp(command[prev_pipe ? pipe_pos[i - 1] + 1 : 0], &command[prev_pipe ? pipe_pos[i - 1] + 1 : 0]) == -1) {
                    perror("Execution error");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Parent process
                wait(NULL);

                if (i > 0) {
                    close(prev_pipe);
                }

                if (i < num_pipes) {
                    prev_pipe = pipe_fd[0];
                    close(pipe_fd[1]);
                }
            }
        }
    } else if (status > 0) {
        int value;
        struct timeval startingTime, endTime;
        gettimeofday(&startingTime, NULL);
        waitpid(status, &value, 0);
        if (WIFEXITED(value)) {
            gettimeofday(&endTime, NULL);
            double span = (endTime.tv_sec - startingTime.tv_sec) + (endTime.tv_usec - startingTime.tv_usec) / 1000000.0;
            struct rusage usage;
            if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
                double memory_used = (double)usage.ru_maxrss / 1024.0;
                addToHistory(history, input, status, time(NULL), span, memory_used, historyCount);
                historyCount++;
            } else {
                perror("getrusage");
            }
        } else {
            perror("CHILD PROCESS NOT TERMINATED (AN ERROR OCCURRED!!!)");
        }
    }
    return 0;
}

// FUNCTION FOR LAUNCHING THE PROCESS
int launch(char *input) {
    // Check if the input contains pipes
    if (strchr(input, '|') != NULL) {
        return create_process(input);
    } else {
        // No pipes, execute the single command
        char *command[MAX_INPUT_SIZE];
        int count = 0;
        char *token = strtok(input, " ");
        while (token != NULL) {
            command[count++] = token;
            token = strtok(NULL, " \n");
        }
        command[count] = NULL;
        
        int status = fork(); // CREATING A PROCESS

        if (status < 0) {
            printf("SOMETHING WENT WRONG!!!!\n");
        } else if (status == 0) {
            // Child process
            if (execvp(command[0], command) == -1) {
                perror("Execution error");
                exit(EXIT_FAILURE);
            }
        } else if (status > 0) {
            // Parent process
            int value;
            struct timeval startingTime, endTime;
            gettimeofday(&startingTime, NULL);
            waitpid(status, &value, 0);
            if (WIFEXITED(value)) {
                gettimeofday(&endTime, NULL);
                double span = (endTime.tv_sec - startingTime.tv_sec) + (endTime.tv_usec - startingTime.tv_usec) / 1000000.0;
                struct rusage usage;
                if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
                    double memory_used = (double)usage.ru_maxrss / 1024.0;
                    addToHistory(history, input, status, time(NULL), span, memory_used, historyCount);
                    historyCount++;
                } else {
                    perror("getrusage");
                }
            } else {
                perror("CHILD PROCESS NOT TERMINATED (AN ERROR OCCURRED!!!)");
            }
        }
        return 0;
    }
}

// MAIN FUNCTION
int main() {
    printf("HELLO!! WELCOME TO CUSTOMIZED C SHELL\n");
    int status;
    char input[MAX_INPUT_SIZE];
    signal(SIGINT, handler); //PASSING THE SIGNAL
    do {
        printf("My_Shell--> ");
        fgets(input, sizeof(input), stdin);
        //ERROR HANDLING
        if (input == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        //REMOVING BACKSLASH FROM USER INPUT
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "history") == 0) {
            printf("------------------------------\n");
            Show_History(history, historyCount);
        } else if (strcmp(input, "exit") == 0) {
            printf("Exiting the shell...\n");
            break;
        } else {
            launch(input);
        }
    } while (status);
    return 0;
}