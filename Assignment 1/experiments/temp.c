#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NUM_PROCESSES 5

int main() {
    int pipes[NUM_PROCESSES][2];

    // Create pipes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Fork child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Child process
            close(pipes[i][0]); // Close read end in child process
            close(pipes[(i + 1) % NUM_PROCESSES][1]); // Close write end to next process

            char message[50];
            snprintf(message, sizeof(message), "Hello from process %d!\n", i);

            // Read from the previous process
            ssize_t bytesRead = read(pipes[(i + NUM_PROCESSES - 1) % NUM_PROCESSES][0], message, sizeof(message));
            
            if (bytesRead > 0) {
                message[bytesRead] = '\0'; // Null-terminate the received message
                printf("%s", message);
            }

            // Write to the next process
            write(pipes[i][1], message, strlen(message));

            close(pipes[i][1]); // Close write end in child process
            close(pipes[(i + NUM_PROCESSES - 1) % NUM_PROCESSES][0]); // Close read end from the previous process
            exit(EXIT_SUCCESS);
        }
    }

    // Close unused pipe ends in the parent process
    for (int i = 0; i < NUM_PROCESSES; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    return 0;
}