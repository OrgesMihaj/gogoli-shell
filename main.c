
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


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

// Core functions

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

    int bufferSize = 64;
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
 *  3. Load and execute the file that contains the command.
 *
 *  4. Can't fork, error occured.
 *
 *  5. Wait for state changes in a child of the calling process, and
 *     obtain information about the child whose state has changed.
 *
 *     WUNTRACED: return if a child has stopped
 *     WIFEXITED: returns true if the child terminated normally
 *     WIFSIGNALED: returns true if the child process was terminated
 *                  by a signal.
 *
 *     linux.die.net/man/2/waitpid
 * */
int processLaunch(char ** args) {
    pid_t pid; /* [1] */
    int status;

    pid = fork(); /* [2] */

    if (pid == 0) {
        execvp(args[0], args); /* [3] */

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("gogoli"); /* [4] */
    } else {

        // Continue until the child process is terminated normally
        // or the child process was terminated by a signal.
        do {
            waitpid(pid, &status, WUNTRACED); /* [5] */

        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
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
int execute(char ** args) {

    char * builtInCommandsName[2] = { "cd", "exit" };

    int (* builtInCommandsFunctions[]) (char **) = { /* [1] */
        &changeDirectory, &terminateShell
    };

    for (int i = 0; i < 2; ++i) {
        if (strcmp(args[0], builtInCommandsName[i]) == 0) { /* [2] */
            return (*builtInCommandsFunctions[i])(args); /* [3] */
        }
    }

    return processLaunch(args); /* [4] */
}


void bootstrap() {
    char * line;
    char ** args;
    int status;

    do {
        printf("gogoli# ");

        line = readCommand();
        args = getArguments(line);
        status = execute(args);

        free(line);
        free(args);

    } while (status != 0);
}

int main(int argc, char** argv) {

    bootstrap();

    return 0;
}