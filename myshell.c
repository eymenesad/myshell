#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_ALIAS_SIZE 64

char* PATH;

// Function to parse input command
void parseInput(char* input, char** command, char** args) {
    *command = strtok(input, " ");
    *args = strtok(NULL, " ");
}

// Function to execute a command
void executeCommand(char* command, char* args, int background, char* outputFile, int append) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
    } else if (pid == 0) {  // Child process
        if (outputFile != NULL) {
            int fd;
            if (append) {
                fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } else {
                fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }

            if (fd == -1) {
                perror("Error opening output file");
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // Execute the command
        execlp(command, command, args, (char *)NULL);

        // If execlp fails
        perror("Execution failed");
    } else {  // Parent process
        if (!background) {
            waitpid(pid, NULL, 0);
        }
        // Append the executed command to the history file
        FILE* historyFile = fopen(".myshell_history", "a");
        if (historyFile == NULL) {
            perror("Error opening history file for appending");
        } else {
            fprintf(historyFile, "%s\n", args ? args : command);
            fclose(historyFile);
        }
    }
}

// Function to get the number of processes being executed
int getProcessCount() {
    FILE* processFile = popen("ps aux | wc -l", "r");
    if (processFile == NULL) {
        perror("Error counting processes");
    }

    int count;
    fscanf(processFile, "%d", &count);
    pclose(processFile);

    // Subtract 1 to exclude the header line
    return count - 1;
}

// Function to get the last executed command
void getLastCommand(char* lastCommand) {
    FILE* historyFile = fopen(".myshell_history", "r");
    if (historyFile == NULL) {
        perror("Error opening history file");
        return;
    }

    // Read the last line from the history file
    while (fgets(lastCommand, MAX_INPUT_SIZE, historyFile) != NULL) {
        // Empty loop body
    }

    fclose(historyFile);

    // Remove newline character
    lastCommand[strcspn(lastCommand, "\n")] = 0;
}

// Function to implement the "bello" command
void belloCommand() {
    char lastCommand[MAX_INPUT_SIZE];
    getLastCommand(lastCommand);

    printf("1. Username: %s\n", getenv("USER"));
    printf("2. Hostname: %s\n", getenv("HOSTNAME"));
    printf("3. Last Executed Command: %s\n", lastCommand);
    printf("4. TTY: %s\n", ttyname(fileno(stdout)));
    printf("5. Current Shell Name: %s\n", getenv("SHELL"));
    printf("6. Home Location: %s\n", getenv("HOME"));

    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    printf("7. Current Time and Date: %s", asctime(timeinfo));

    printf("8. Current number of processes being executed: %d\n", getProcessCount());
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *command, *args;
    char alias[MAX_ALIAS_SIZE][MAX_INPUT_SIZE];
    int aliasCount = 0;

    // Get the PATH variable
    PATH = getenv("PATH");
    setenv("HOSTNAME", "your_hostname", 1);
    while (1) {
        // Print prompt
        printf("%s@%s %s --- ", getenv("USER"), getenv("HOSTNAME"), getcwd(NULL, 0));

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;  // Exit on EOF
        }

        // Remove newline character
        input[strcspn(input, "\n")] = 0;
         // Check for exit command
        if (strcmp(input, "exit") == 0) {
            break;  // Exit the loop and terminate the shell
        }

        // Check for alias command
        if (strncmp(input, "alias", 5) == 0) {
            sscanf(input, "alias %s = \"%[^\"]\"", alias[aliasCount], alias[aliasCount]);
            aliasCount++;
            continue;
        }

        // Check for bello command
        if (strcmp(input, "bello") == 0) {
            // Implement bello command
            belloCommand();
            continue;
        }

        // Parse input command
        parseInput(input, &command, &args);

        // Check for alias substitution
        for (int i = 0; i < aliasCount; i++) {
            if (strcmp(command, alias[i]) == 0) {
                parseInput(alias[i], &command, &args);
                break;
            }
        }
        // Check for background processing
        int background = 0;
        if (args != NULL && strcmp(args, "&") == 0) {
            background = 1;
            args = NULL;
        }

        // Check for redirection
        char* outputFile = NULL;
        int append = 0;
        if (args != NULL && (strcmp(args, ">") == 0 || strcmp(args, ">>") == 0)) {
            outputFile = strtok(NULL, " ");
            append = (strcmp(args, ">>") == 0);
            args = NULL;
        }

        // Execute the command
        executeCommand(command, args, background, outputFile, append);
    }

    return 0;
}
