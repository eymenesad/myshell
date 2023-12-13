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
    #define MAX_ALIAS_SIZE 1024



    char* PATH;

    // Data structure to store aliases
    typedef struct {
        char alias[MAX_ALIAS_SIZE][MAX_INPUT_SIZE];
        char command[MAX_ALIAS_SIZE][MAX_INPUT_SIZE];
        int count;
    } AliasList;

    AliasList aliases;


    void parseInput(char* input, char*** commandArgs) {
    int i = 0;
    // Allocate memory for an array of strings to store command and arguments
    *commandArgs = malloc((MAX_ARG_SIZE + 1) * sizeof(char*));

    char* token = strtok(input, " ");
  
    while (token != NULL && i < MAX_ARG_SIZE) {
        
        (*commandArgs)[i] = strdup(token);
       
        token = strtok(NULL, " ");
        
        i++;
    }
    
    // Set the last element to NULL to indicate the end of the array
    (*commandArgs)[i] = NULL;
      
    }

    void saveAliasesToFile() {
        FILE* aliasFile = fopen("aliases.txt", "w");
        if (aliasFile == NULL) {
            return;
        }

        for (int i = 0; i < aliases.count; i++) {
            fprintf(aliasFile, "%s = \"%s\"\n", aliases.alias[i], aliases.command[i]);
        }

        fclose(aliasFile);
    }



    // Function to add alias to the list
    void addAlias(char* alias, char* command) {
        if (aliases.count < MAX_ALIAS_SIZE) {
            strcpy(aliases.alias[aliases.count], alias);
            strcpy(aliases.command[aliases.count], command);
            aliases.count++;
            
        } else {
            fprintf(stderr, "Maximum number of aliases reached\n");
        }
    }




    void loadAliasesFromFile() {
        FILE* aliasFile = fopen("aliases.txt", "r");
        if (aliasFile == NULL) {
            return;
        }

        char alias[MAX_INPUT_SIZE], command[MAX_INPUT_SIZE];
        while (fscanf(aliasFile, " %[^= ] = \"%[^\"]\"", alias, command) == 2) {
            int aliasExists = 0;
            for (int i = 0; i < aliases.count; i++) {
                if (strcmp(alias, aliases.alias[i]) == 0) {
                    aliasExists = 1;
                    break;
                }
            }

            if (!aliasExists) {
                addAlias(alias, command);
            }
        }

        fclose(aliasFile);
    }

    // Function to substitute alias in the command
    int substituteAlias(char* input, char*** commandArgs) {
        // Load aliases from file during initialization
        loadAliasesFromFile();

        for (int i = 0; i < aliases.count; i++) {
            if (strcmp(input, aliases.alias[i]) == 0) {
                // Parse the substituted command
                char substitutedCommand[MAX_INPUT_SIZE];
                strcpy(substitutedCommand, aliases.command[i]);

                // Clear existing commandArgs
                    if (*commandArgs != NULL) {
                        int j = 0;
                        while ((*commandArgs)[j] != NULL) {
                            free((*commandArgs)[j]);
                            j++;
                        }
                        free(*commandArgs);
                    }

                // Parse the substituted command into commandArgs
                parseInput(substitutedCommand, commandArgs);

                return 1; // Alias substitution successful
            }
        }

        return 0; // No alias substitution
    }

 

        // Function to execute a command
    void executeCommand(char* command, char** args, int background, char* outputFile, int append, int isAlias, int invertOrder) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("Fork failed");
        } else if (pid == 0) {  // Child process
            // Check for redirection
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
            if(!invertOrder) {
            // Execute the command in the child process
                if (isAlias) {
                    execvp(args[0], args);
                } else {
                    execvp(command, args);
                }

                // If execlp fails
                perror("Execution failed");
                exit(1);
            }

            // Check for invert order and background process
            if (invertOrder) {
                pid_t grandchild_pid = fork();

                if (grandchild_pid == -1) {
                    perror("Grandchild Fork failed");
                } else if (grandchild_pid == 0) {  // Grandchild process
                    // Set up pipe for communication between grandchild and child
                    int pipefd[2];
                    if (pipe(pipefd) == -1) {
                        perror("Pipe failed");
                        exit(1);
                    }

                    // Fork another process to handle invert order
                    pid_t invert_pid = fork();

                    if (invert_pid == -1) {
                        perror("Invert Fork failed");
                    } else if (invert_pid == 0) {  // Invert order process
                        // Close the write end of the pipe
                        close(pipefd[1]);

                        // Read from the pipe and invert the content
                        char buffer[MAX_INPUT_SIZE];
                        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer));

                        for (ssize_t i = bytesRead - 1; i >= 0; i--) {
                            putchar(buffer[i]);
                        }

                        close(pipefd[0]);
                        exit(0);
                    } else {  // Child process
                        // Close the read end of the pipe
                        close(pipefd[0]);

                        // Redirect stdout to the write end of the pipe
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);

                        // Execute the command in the background
                        if (isAlias) {
                            execvp(args[0], args);
                        } else {
                            execvp(command, args);
                        }

                        perror("Execution failed");
                        exit(1);
                    }
                } else {  // Child process
                    // Wait for the grandchild process to finish
                    int status;
                    waitpid(grandchild_pid, &status, 0);
                    // Execute the command in the child process
                    /*if (isAlias) {
                        execvp(args[0], args);
                    } else {
                        execvp(command, args);
                    }

                    // If execlp fails
                    perror("Execution failed");*/
                    exit(1);
                }
                    // Exit the child process
                  
                
            } else {
                // Execute the command in the child process
                if (isAlias) {
                    execvp(args[0], args);
                } else {
                    execvp(command, args);
                }

                // If execlp fails
                perror("Execution failed");
                exit(1);
            }
        } else {  // Parent process
            if (!background) {
                int status;
                waitpid(pid, &status, 0);
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
    void belloCommand(int processCount) {
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
        // Handle terminated child processes (zombies)
        // Check for terminated child processes and update processCount
        pid_t terminated_child;
        do {
            terminated_child = waitpid(-1, NULL, WNOHANG);
            if (terminated_child < 0) {
                processCount--;
            }
        } while (terminated_child > 0);
        if(processCount < 0) {
            processCount = 0;
        }                            
        printf("8. Current number of processes being executed: %d\n", processCount);
    }

    int main() {
        char input[MAX_INPUT_SIZE];
        
        char** commandArgs=NULL;
        // Get the PATH variable
        PATH = getenv("PATH");
        setenv("HOSTNAME", "your_hostname", 1);
        int processCount = 0;
        
        while (1) {
            loadAliasesFromFile();
            int background = 0;
            int argCount = 0;
            char* outputFile = NULL;
            int append = 0;
            int invertOrder = 0;
            // Print prompt
            printf("%s@%s %s --- ", getenv("USER"), getenv("HOSTNAME"), getcwd(NULL, 0));

            // Read user input
            if (fgets(input, sizeof(input), stdin) == NULL) {
                break;  // Exit on EOF
            }

            // Remove newline character
            input[strcspn(input, "\n")] = 0;
            // Check for alias creation
            if (strncmp(input, "alias", 5) == 0) {
                char alias[MAX_INPUT_SIZE], command[MAX_INPUT_SIZE];
                if (sscanf(input, "alias %s = \"%[^\"]\"", alias, command) == 2) {
                    addAlias(alias, command);
                    // Save aliases to file after adding a new alias
                    saveAliasesToFile();
                    continue;
                }
                
                
                
                
                else {
                    fprintf(stderr, "Invalid alias syntax\n");
                    continue;
                }
            
            }
            // Parse input command
            parseInput(input,  &commandArgs);

            // Check if commandArgs is NULL, which may happen due to an error in parseInput
            if (commandArgs == NULL || commandArgs[0] == NULL) {
                continue;
            }
            // Check for alias substitution
            if (substituteAlias(input, &commandArgs)) {
                //check for > or >> or >>> in the substituted command
                // Count the number of arguments
                while (commandArgs[argCount] != NULL) {
                    argCount++;
                }

                
                // Check for background processing
                if (argCount > 1 && strcmp(commandArgs[argCount-1], "&") == 0) {
                    background = 1;
                    commandArgs[argCount - 1] = NULL;  // Remove the "&" from the arguments
                    argCount--;
                    processCount++;
                }

                // Check for redirection
            
                if (argCount > 1 && (strcmp(commandArgs[argCount - 2], ">") == 0)) {
                    outputFile = commandArgs[argCount - 1];
                    commandArgs[argCount - 2] = NULL;  // Remove the ">" or ">>" and the filename
                    argCount -= 2;
                    append = 0;
                }
                if (argCount > 1 &&  strcmp(commandArgs[argCount - 2], ">>") == 0) {
                    
                    outputFile = commandArgs[argCount - 1];
                    
                    commandArgs[argCount - 2] = NULL;  // Remove the ">" or ">>" and the filename
                    argCount -= 2;
                    append = 1;
                }
                
                // Check for re-redirection (">>>")
                if (argCount > 1 && strcmp(commandArgs[argCount - 2], ">>>") == 0) {
                    outputFile = commandArgs[argCount - 1];
                    invertOrder = 1;
                    commandArgs[argCount - 2] = NULL;  // Remove the ">>>" and the filename
                    argCount -= 2;
                }
                    
                    
                    
                // Alias substitution successful, continue to the next iteration
                executeCommand(commandArgs[0], commandArgs, background, outputFile, append, 1, invertOrder);

                // Free allocated memory for commandArgs after parsing and checking for alias substitution
                for (int i = 0; i < argCount; i++) {
                    free(commandArgs[i]);
                }
                free(commandArgs);

                // Set commandArgs to NULL to avoid reuse issues
                commandArgs = NULL;

                continue;
            }
            
            
            // Count the number of arguments
            while (commandArgs[argCount] != NULL) {
                
                argCount++;
                
            }

            
            // Check for background processing
            if (argCount > 1 && strcmp(commandArgs[argCount-1], "&") == 0) {
                background = 1;
                commandArgs[argCount - 1] = NULL;  // Remove the "&" from the arguments
                argCount--;
                processCount++;
            }
             // Check for bello command
            if (strcmp(input, "bello") == 0) {
                int flagForredirection = 0;
                // Check for redirection
                if (argCount > 1 && (strcmp(commandArgs[argCount - 2], ">") == 0)) {
                    outputFile = commandArgs[argCount - 1];
                    commandArgs[argCount - 2] = NULL;  // Remove the ">" or ">>" and the filename
                    argCount -= 2;
                    append = 0;
                    flagForredirection = 1;
                }
                if (argCount > 1 &&  strcmp(commandArgs[argCount - 2], ">>") == 0) {
                    outputFile = commandArgs[argCount - 1];
                    commandArgs[argCount - 2] = NULL;  // Remove the ">" or ">>" and the filename
                    argCount -= 2;
                    append = 1;
                    flagForredirection = 1;    
                }

                // Check for re-redirection (">>>")
                if (argCount > 1 && strcmp(commandArgs[argCount - 2], ">>>") == 0) {
                    outputFile = commandArgs[argCount - 1];
                    invertOrder = 1;
                    commandArgs[argCount - 2] = NULL;  // Remove the ">>>" and the filename
                    argCount -= 2;
                    flagForredirection = 1;
                }

                if (flagForredirection == 1) {
                    pid_t pid = fork();

                    if (pid == -1) {
                        perror("Fork failed");
                    } else if (pid == 0) {  // Child process
                        int fd;
                        if (append) {
                            fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                        } else {
                            fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        }

                        if (fd == -1) {
                            perror("Error opening output file");
                            exit(1);
                        }

                        // Redirect stdout to the file
                        dup2(fd, STDOUT_FILENO);
                        close(fd);

                        // Execute belloCommand
                        belloCommand(processCount);

                        // Exit the child process
                        exit(0);
                    } else {  // Parent process
                        if (!background) {
                            int status;
                            waitpid(pid, &status, 0);
                        }
                        // Rest of the code for the parent process (history file, etc.)
                        // Append the executed command to the history file
                        FILE* historyFile = fopen(".myshell_history", "a");
                        if (historyFile == NULL) {
                            perror("Error opening history file for appending");
                        } else {
                            fprintf(historyFile, "%s", commandArgs[0]);
                            // Append arguments to the history file
                            for (int i = 1; commandArgs[i] != NULL; i++) {
                                fprintf(historyFile, " %s", commandArgs[i]);
                            }
                            fprintf(historyFile, "\n");
                            fclose(historyFile);
                        }
                    }

                    // Free allocated memory for commandArgs
                    for (int i = 0; i < argCount; i++) {
                        free(commandArgs[i]);
                    }
                    free(commandArgs);
                    commandArgs = NULL;

                    background = 0;
                    outputFile = NULL;
                    append = 0;
                    continue;
                } else {
                    // Execute belloCommand without redirection
                    belloCommand(processCount);
                    continue;
                }
            }

            // Check for redirection
           
            if (argCount > 1 && (strcmp(commandArgs[argCount - 2], ">") == 0)) {
                outputFile = commandArgs[argCount - 1];
                commandArgs[argCount - 2] = NULL;  // Remove the ">" or ">>" and the filename
                argCount -= 2;
                append = 0;
            }
            if (argCount > 1 &&  strcmp(commandArgs[argCount - 2], ">>") == 0) {
                
                outputFile = commandArgs[argCount - 1];
                
                commandArgs[argCount - 2] = NULL;  // Remove the ">" or ">>" and the filename
                argCount -= 2;
                append = 1;
            }
            
            // Check for re-redirection (">>>")
            if (argCount > 1 && strcmp(commandArgs[argCount - 2], ">>>") == 0) {
                outputFile = commandArgs[argCount - 1];
                invertOrder = 1;
                commandArgs[argCount - 2] = NULL;  // Remove the ">>>" and the filename
                argCount -= 2;
            }
            
            // Check for exit command
            if (strcmp(input, "exit") == 0) {
                break;  // Exit the loop and terminate the shell
            }
            
            // Execute the command
            executeCommand(commandArgs[0], commandArgs, background, outputFile, append,0, invertOrder);
            // Free allocated memory for commandArgs
            for (int i = 0; i < argCount; i++) {
                free(commandArgs[i]);
            }
            free(commandArgs);
            commandArgs = NULL;
            background = 0;
            outputFile = NULL;
            append = 0;
            
    }
    }
