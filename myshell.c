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
void parseInput(char* input, char*** commandArgs) {
    int i = 0;
    char* token = strtok(input, " ");
    
    // Allocate memory for an array of strings to store command and arguments
    *commandArgs = malloc((MAX_ARG_SIZE + 1) * sizeof(char*));

    while (token != NULL && i < MAX_ARG_SIZE) {
        (*commandArgs)[i] = strdup(token);
        token = strtok(NULL, " ");
        i++;
    }

    // Set the last element to NULL to indicate the end of the array
    (*commandArgs)[i] = NULL;
}


// Function to execute a command
void executeCommand(char* command, char** args, int background, char* outputFile, int append) {
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
        execvp(command, args);

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
            fprintf(historyFile, "%s", command);
            // Append arguments to the history file
            for (int i = 1; args[i] != NULL; i++) {
                fprintf(historyFile, " %s", args[i]);
            }
            fprintf(historyFile, "\n");
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
    //char *command, *args;
    
    char** commandArgs;
    //char alias[MAX_ALIAS_SIZE][MAX_INPUT_SIZE];
    //int aliasCount = 0;

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
       /* if (strncmp(input, "alias", 5) == 0) {
            sscanf(input, "alias %s = \"%[^\"]\"", alias[aliasCount], alias[aliasCount]);
            aliasCount++;
            continue;
        }*/

        // Check for bello command
        if (strcmp(input, "bello") == 0) {
            // Implement bello command
            belloCommand();
            continue;
        }

        // Parse input command
        parseInput(input,  &commandArgs);

        // Check for alias substitution
        /*for (int i = 0; i < aliasCount; i++) {
            if (strcmp(command, alias[i]) == 0) {
                parseInput(alias[i], &command, &args);
                break;
            }
        }*/
        // Check for background processing
        int background = 0;
        int argCount = 0;
        
         // Count the number of arguments
        while (commandArgs[argCount] != NULL) {
            argCount++;
        }

        // Check for background processing
        if (argCount > 1 && strcmp(commandArgs[argCount - 1], "&") == 0) {
            background = 1;
            commandArgs[argCount - 1] = NULL;  // Remove the "&" from the arguments
        }

         // Check for redirection
        char* outputFile = NULL;
        int append = 0;
        if (argCount > 1 && (strcmp(commandArgs[argCount - 2], ">") == 0 || strcmp(commandArgs[argCount - 2], ">>") == 0)) {
            outputFile = commandArgs[argCount - 1];
            append = (strcmp(commandArgs[argCount - 2], ">>") == 0);
            commandArgs[argCount - 2] = NULL;  // Remove the ">" or ">>" and the filename
        }
        
        // Execute the command
        executeCommand(commandArgs[0], commandArgs, background, outputFile, append);

        // Free allocated memory for commandArgs
        for (int i = 0; i < argCount; i++) {
            free(commandArgs[i]);
        }
        free(commandArgs);
    }

    return 0;
}
