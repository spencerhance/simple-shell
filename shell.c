#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Size of the input buffer, defaults to 80
size_t buffer_size = 80;

// Child pid for signal handler
// We are keeping track of this so the child process can be killed if
// the main process is killed, or exit is called
pid_t pid = -1;

// Definitions
void signalHandler(int sig_id);
void mainLoop();
char* readLine();
char** parseLine(char *line, int *num_tokens);
int executeCommand(char **args);
int launchProcess(char **args);

// Shell builtins
int simpleShellCd(char **args);
int simpleShellHelp(char **args);
int simpleShellEcho(char **args);
int simpleShellExit(char **args);

// Built in commands
const int num_builtins = 4;
char *builtin_commands[] = {
    "cd",
    "help",
    "echo",
    "exit"};

// Array of function pointers to builtins
typedef int (*builtin_pointer_t) (char **);
builtin_pointer_t builtin_pointers[] = {
    &simpleShellCd,
    &simpleShellHelp,
    &simpleShellEcho,
    &simpleShellExit};

// Main
int main()
{
    // Register signal handler for SIGINT
    signal(SIGINT, signalHandler);

    // Run Shell
    mainLoop();
    return 0;
}


// Signal handler to handle ^C
void signalHandler(int sig_id)
{
    printf("\nsimple-shell terminated\n");
    
    // Kill child process
    if (pid != -1)
        kill(pid, SIGINT);
   
    // Exit current process
    exit(0);
}




//
// Shell Functions
//

// The main shell loop
void mainLoop()
{
    // Start the shell prompt
    printf("simple-shell> ");
    
    // Shell pointers
    char *line;
    char **tokens;
    int num_tokens;
    int status = 1;

    // Shell loop
    while(status)
    {
        // Get line from CLI
        line = readLine();

        // Execute line if something is read
        if (line != NULL)
        {
            // Parse line into tokens
            tokens = parseLine(line, &num_tokens);

            // Execute tokens
            status = executeCommand(tokens);
            
            // Update prompt
            printf("simple-shell> ");
        }

        // Free memory
        free(line);
        free(tokens);
    }
    
    // Extra newline after program is killed
    printf("\n");

    return;
}


// Read a line from the CLI
char* readLine()
{
    // Allocate buffer
    char *buffer = malloc(sizeof(char) * buffer_size);
    
    // Read line
    // Buffer will be realloc'd if necessary by getline
    ssize_t num_characters_read = getline(&buffer, &buffer_size, stdin);

    return buffer;
}


// Tokenize a given line into commands
char** parseLine(char *line, int *num_tokens)
{
    //printf("Parse %s into tokens here and print them out\n", data);
    
    // Token buffer to store tokens 
    // Using a static buffer for now to ignore using realloc
    // Also since we have a max line size, we can have a maximum
    // of buffer_size/2 tokens
    char** buffer = malloc(sizeof(char*) * (buffer_size / 2));
    
    // Initialize array to NULL
    memset(buffer, 0, buffer_size/2);
    
    // Index in the buffer
    int i = 0;
    
    // Copy the line into args since we will be mutating the string
    char* args = strdup(line);

    // Get first token
    char *token = strtok(args, " \n");

    // Get the rest of the tokens
    while (token != NULL)
    {
        // Add token to our buffer
        buffer[i] = token;
        
        // Get next token
        token = strtok(NULL, " \n");
        
        // Update index
        i++;
    }

    // Update the number of tokens
    *num_tokens = i;

    return buffer;
}


// Start new process
// Borrowed from https://brennan.io/2015/01/16/write-a-shell-in-c/
int startProcess(char **args)
{
    // Setup
    //pid_t pid;
    pid_t wait_pid;
    int child_status;

    // Get fork pid
    pid = fork();
    
    // Child
    if (pid == 0)
    {
        // Execute args
        // Error if failure
        // execvp automatically finds the command in the path
        if (execvp(args[0], args) == -1)
        {
            perror("simple-shell");
            exit(1);
        }
    }

    // Error
    else if (pid < 0)
    {
        perror("simple-shell");
        exit(1);    
    }

    // Parent
    // Wait for child
    else
    {
        // Wait until either exits or killed
        wait_pid = waitpid(pid, &child_status, WUNTRACED);
        while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status))
            wait_pid = waitpid(pid, &child_status, WUNTRACED);

        // Reset pid
        pid = -1;
    }
    
    return 1;
}


// Execute a given command
int executeCommand(char **args)
{
    // Empty command
    if (args[0] == NULL)
        return 1;
    
    // Try to find builtin and execute it
    for (int i = 0; i < num_builtins; i++)
    {
        // String match on builtin function
        if (strcmp(args[0], builtin_commands[i]) == 0)
        {
            return (*builtin_pointers[i])(args);
        }
    }

    // Couldn't find builtin, launch a new process and look for executable
    //printf("Couldn't find builtin\n");
    return startProcess(args);
}




//
// Built in functions
//

// simple cd using chdir
int simpleShellCd(char **args)
{
    // cd to dir in args
    
    if (args[1] != NULL)
    {
        int status = chdir(args[1]);

        if (status == -1)
        {
            printf("cd failed, check your path\n");
        }
    }
    
    return 1;
}


// Simple help function
int simpleShellHelp(char **args)
{
    printf("Commands available: cd, help, echo, exit\n");
    return 1;
}


// Simple echo function
int simpleShellEcho(char **args)
{
    // Iterate through all of the args and print them
    int i = 1;
    while (args[i] != NULL && args[i] != '\0')
    {
        // Printing the arg with a space since it was split out
        printf("%s ", args[i]);
        i++;
    }

    // New line after print
    printf("\n");    

    return 1;
}


// Simple exit
// Just ends main loop
int simpleShellExit(char **args)
{
    return 0;
}
