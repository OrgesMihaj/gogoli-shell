
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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


int processLaunch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("lsh");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}



void bootstrap() {
    char * line;
    char ** args;
    int status;

    do {
        printf("gogoli# ");

        line = readCommand();
        args = getArguments(line);
        status = processLaunch(args);

        free(line);
        free(args);

    } while (status);
}

int main(int argc, char** argv) {

    bootstrap();

    return 0;
}