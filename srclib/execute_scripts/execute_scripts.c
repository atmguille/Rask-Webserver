#include "execute_scripts.h"
#include "../logging/logging.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

// Enum to read and write in pipes
enum { READ = 0, WRITE = 1 };
#define MAX_SIZE 256

/**
 * Executes script with the specified interpreter
 * @param interpreter
 * @param path
 * @param stdin_args
 * @param len_stdin_args
 * @return string that must be freed with the script output
 */
char *_execute_script(char *interpreter, char *path, const char *stdin_args, int len_stdin_args) {
    int stdin_pipe[2];
    int stdout_pipe[2];
    pid_t pid;
    char *output;

    output = (char *)malloc(MAX_SIZE * sizeof(char));
    if (output == NULL) {
        print_error("failed to allocate memory for return string");
        return NULL;
    }

    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        print_error("failed to create pipe: %s", strerror(errno));
        return NULL;
    }

    pid = fork();
    if (pid == 0) { // Child
        // Close unused pipe ends
        close(stdin_pipe[WRITE]);
        close(stdout_pipe[READ]);

        if (dup2(stdin_pipe[READ], STDIN_FILENO) == -1 ||
            dup2(stdout_pipe[WRITE], STDOUT_FILENO) == -1 ||
            dup2(stdout_pipe[WRITE], STDERR_FILENO) == -1) {

            print_error("failed to duplicate file descriptors: %s", strerror(errno));
            exit(1);
        }

        execlp(interpreter, interpreter, path, NULL);

        print_error("excelp failed");
        exit(1);

    } else if (pid > 0) { // Father
        // Close unused pipe ends
        close(stdin_pipe[READ]);
        close(stdout_pipe[WRITE]);

        if (write(stdin_pipe[WRITE], stdin_args, len_stdin_args) == -1) {
            print_error("failed to write stdin_args to child: %s", strerror(errno));
            kill(pid, SIGKILL);
            wait(NULL);
            return NULL;
        }

        // Write end must be closed so EOF is sent to child
        close(stdin_pipe[WRITE]);

        if (read(stdout_pipe[READ], output, MAX_SIZE) == -1) {
            print_error("failed to read return string from child: %s", strerror(errno));
            kill(pid, SIGKILL);
            wait(NULL);
            return NULL;
        }
        close(stdout_pipe[READ]);
        wait(NULL);
        return output;

    } else {
        print_error("failed to create child to execute script");
        return NULL;
    }

}

char *execute_python_script(char *path, const char *stdin_args, int len_stdin_args) {
    return _execute_script("python3", path, stdin_args, len_stdin_args);
}

char *execute_php_script(char *path, const char *stdin_args, int len_stdin_args) {
    return _execute_script("php", path, stdin_args, len_stdin_args);
}