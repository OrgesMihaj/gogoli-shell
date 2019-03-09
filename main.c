#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

// Built-in functions

int changeDirectory(char ** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    } else {
        chdir(args[1]);
    }

    return 1;
}

int terminateShell(char ** args) {
    return 0;
}

/*
 * Check if the command is running in background
 *
 * */
bool isBackgroundProcess(char ** args){
    int i = 0;

    while (args[i] != NULL){
        if (strcmp( args[i],"&")==0) return true;

        i++;
    }
    return false;
}

/*
 * Print the absolute pathname of the current working directory
 *
 * */
void printPrompt() {
    char pathname[1024];

    if (getcwd(pathname, sizeof(pathname)) != NULL) {
        printf("Gogoli(%s)> ", pathname);
    } else{
        printf("WhereAmI> ");
    }
}

/*
 * Read user input.
 *
 * 1. Represents the size of an allocated block of memory.`
 *    The `ssize_t` data type accepts signed numbers.
 *
 * 2. Line-input function.
 *    buffer: the first character position where the input string
 *    will be stored.
 *
 *    size: the address of the variable that holds the size of
 *    the input buffer.
 *
 *    stdin: the input file handle.
 *
 * */
char * readCommand() {
    char * line = NULL;
    ssize_t bufferSize = 0; /* [1] */

    getline(&line, (size_t *) &bufferSize, stdin); /* [2] */

    return line;
}

/*
 * Parse user command into a list of arguments.
 *
 * Tokens are building blocks of C programming, those are
 * identifier, variable, special symbols, etcs.
 *
 * 1. Allocates the requested memory and returns a pointer to it.
 *
 * 2. A sequence of calls to this function split str into tokens,
 *    which are sequences of contiguous characters separated by any
 *    of the characters that are part of delimiters.
 *
 *    For instance:
 *    char * line = "ls -a";
 *    token = strtok(line, delimiters); will return the first token (`ls`)
 *
 * 3. Needed to tokenize the whole command (user input).
 *
 * 4. Assign the token to the `tokens` array.
 *
 * 5. Reallocate the tokens memory size if the command is longer
 *    than the buffer size.
 *
 *
 * */
char ** getArguments(char * line) {

    int bufferSize = 1024;
    int position   = 0;
    char * delimiters = " \t\r\n\a";

    char ** tokens = malloc(bufferSize * sizeof(char *)); /* [1] */
    char  * token;

    token = strtok(line, delimiters); /* [2] */

    while (token != NULL) { /* [3] */

        tokens[position] = token;  /* [4] */
        position++;

        if (position >= bufferSize) {
            bufferSize += bufferSize;

            tokens = realloc(tokens, bufferSize * sizeof(char *)); /* [5] */
        }


        token = strtok(NULL, delimiters); /* [3] */

    }

    tokens[position] = NULL;

    return tokens;
}

char ** getArgumentsByAmpersand(char * line){
    int bufferSize = 1024;
    int position   = 0;

    char ** tokens = malloc(bufferSize * sizeof(char *)); /* [1] */
    char  * token;

    token = strtok_r(line, "&", &line); /* [2] */

    while (token != NULL) { /* [3] */

        tokens[position] = token;  /* [4] */
        position++;

        if (position >= bufferSize) {
            bufferSize += bufferSize;

            tokens = realloc(tokens, bufferSize * sizeof(char *)); /* [5] */
        }

        token = strtok_r(line, "&", &line); /* [3] */
    }

    tokens[position] = NULL;

    return tokens;
}


/*
 * Responsible for launching a program.
 *
 * 1. Signed integer type which is capable of representing a process ID.
 *    gnu.org/software/libc/manual/html_node/Process-Identification.html
 *
 * 2. Creates a new process (child process).
 *
 *    +value: creation of a child process was unsuccessful.
 *         0: returned to the newly created child process.
 *    -value: returned to parent or caller. value = ID of newly created
 *            child process.
 *
 *  3. In case that the user enters a command followed by ampersand (&),
 *     remove the ampersand and get the command args.
 *
 *  4. Load and execute the file that contains the command.
 *
 *  5. Can't fork, error occured.
 *
 * */
int processLaunch(char ** args, char * lineWithAmpersand) {
    pid_t pid; /* [1] */

    int status;

    pid = fork(); /* [2] */

    if (pid == 0) {

        if (isBackgroundProcess(args)) {
            char ** getCommandWithoutAmpersand = getArgumentsByAmpersand(lineWithAmpersand); /* [3] */
            char ** newArgs = getArguments(getCommandWithoutAmpersand[0]); /* [3] */

            execvp(newArgs[0], newArgs);
        } else{
            execvp(args[0], args); /* [4] */
        }

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("gogoli"); /* [5] */
    } else {

        if (isBackgroundProcess(args)) {
            waitpid(-1, &status, WNOHANG);
        } else {
            wait(NULL);
        }
    }

    return 1;
}


/*
 * Program execution function.
 *
 * 1. Array of built-in functions.
 *
 * 2. Check if the command provided by the user match one of
 *    the built-in functions.
*
*  3. Execute the built-in function.
 *
 * 4. If the user is not asking for a built-in function,
 *    call `processLaunch` function.
 * */
int execute(char ** args, char * lineWithAmpersand) {

    char * builtInCommandsName[2] = { "cd", "exit" };

    int (* builtInCommandsFunctions[]) (char **) = { /* [1] */
            &changeDirectory, &terminateShell
    };

    for (int i = 0; i < 2; ++i) {
        if (strcmp(args[0], builtInCommandsName[i]) == 0) { /* [2] */
            return (*builtInCommandsFunctions[i])(args); /* [3] */
        }
    }

    return processLaunch(args, lineWithAmpersand); /* [4] */
}

/*
 * Retrieve user command and execute it.
 *
 * 1. Deallocate the memory previously allocated.
 *
 * */
void bootstrap() {
    char * command;
    char ** args;
    char commandWithArgs[256];

    int status;

    do {
        printPrompt();

        command = readCommand();
        strcpy(commandWithArgs, command);
        args   = getArguments(command);
        status = execute(args, commandWithArgs);

        free(command); /* [1] */
        free(args);    /* [1] */

    } while (status);
}

int main(int argc, char** argv) {
    /**  The beginning is the most important part of the work. */
    /**/ bootstrap();

    return 0;
}