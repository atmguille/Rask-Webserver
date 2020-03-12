#include "execute_scripts.h"
#include "../logging/logging.h"
#include "../dynamic_buffer/dynamic_buffer.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

// Enum to read and write in pipes
enum { READ = 0, WRITE = 1 };

/**
 * Executes script with the specified interpreter
 * @param interpreter
 * @param path
 * @param stdin_args
 * @param len_stdin_args
 * @return dynamic buffer with the output of the script
 */
DynamicBuffer *_execute_script(char *interpreter, char *path, struct string stdin_args) {
    int stdin_pipe[2];
    int stdout_pipe[2];
    pid_t pid;

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
        exit(EXIT_FAILURE);

    } else if (pid > 0) { // Father
        DynamicBuffer *db = dynamic_buffer_ini(DEFAULT_FD_BUFFER);
        if (db == NULL) {
            print_error("failed to allocate memory for dynamic buffer");
            kill(pid, SIGKILL);
            wait(NULL);
            return NULL;
        }

        // Close unused pipe ends
        close(stdin_pipe[READ]);
        close(stdout_pipe[WRITE]);

        if (write(stdin_pipe[WRITE], stdin_args.data, stdin_args.size) == -1) {
            print_error("failed to write stdin_args to child: %s", strerror(errno));
            close(stdout_pipe[READ]);
            close(stdin_pipe[WRITE]);
            kill(pid, SIGKILL);
            wait(NULL);
            return NULL;
        }

        // Write end must be closed so EOF is sent to child
        close(stdin_pipe[WRITE]);

        dynamic_buffer_append_fd(db, stdout_pipe[READ]);

        close(stdout_pipe[READ]);
        wait(NULL);
        return db;

    } else {
        print_error("failed to create child to execute script");
        return NULL;
    }

}

DynamicBuffer *execute_python_script(char *path, struct string stdin_args) {
    return _execute_script("python3", path, stdin_args);
}

DynamicBuffer *execute_php_script(char *path, struct string stdin_args) {
    return _execute_script("php", path, stdin_args);
}