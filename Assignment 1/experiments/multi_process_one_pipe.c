
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NUM_PROCESSES 8

int main() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Child process
            close(fd[0]); // Close the read end

            char message[50];
            snprintf(message, sizeof(message), "Hello from process %d!\n", i);
            write(fd[1], message, strlen(message));

            close(fd[1]); // Close the write end in the child process
            exit(EXIT_SUCCESS);
        }
    }

    // Parent process
    close(fd[1]); // Close the write end

    char buffer[100];
    ssize_t bytesRead;

    while ((bytesRead = read(fd[0], buffer, sizeof(buffer))) > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the received message
        printf("%s", buffer);
    }

    close(fd[0]); // Close the read end in the parent process

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    return 0;
}