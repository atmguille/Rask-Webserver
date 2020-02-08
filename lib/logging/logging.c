#include <syslog.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

//  LOG_ERR is 1 and LOG_DEBUG is 7, so a smaller numbers means more important logs
int log_limit = LOG_DEBUG;

bool can_use_stdout() {
    // We're going to check whether the stdout fd is open or not
    int stdout_fd = 1;
    return fcntl(stdout_fd, F_GETFD) != -1;
}

bool can_use_stderr() {
    // We're going to check whether the stdout fd is open or not
    int stderr_fd = 2;
    return fcntl(stderr_fd, F_GETFD) != -1;
}

// IMPORTANT: do NOT free the string returned
char* get_time() {
    time_t current_time = time(NULL);
    char* time_str = ctime(&current_time);
    // ctime will put a newline at the end of time_str, which we will now remove
    for (char* iterator = time_str; *iterator; iterator++) {
        if (*iterator == '\n') {
            *iterator = '\0';
            // Since there's only one '\n'
            break;
        }
    }

    return time_str;
}

// Priority names
const char* CRITICAL = "CRITICAL";
const char* ERROR = "ERROR";
const char* WARNING = "WARNING";
const char* INFO = "INFO";
const char* DEBUG = "DEBUG";
const char* UNKNOWN = "UNKNOWN";

const char* get_priority_name(int priority) {
     switch (priority) {
         case LOG_CRIT:
             return CRITICAL;
         case LOG_ERR:
             return ERROR;
         case LOG_WARNING:
             return WARNING;
         case LOG_INFO:
             return INFO;
         case LOG_DEBUG:
             return DEBUG;
         default:
             return UNKNOWN;
     }
}

void print_priority(const char* string, int priority, bool use_stderr) {
    const char* priority_name;

    // Don't log messages less important than the limit
    if (priority > log_limit) {
        return;
    }

    priority_name = get_priority_name(priority);

    if ((use_stderr && can_use_stderr()) || (!use_stderr && can_use_stdout())) {
        FILE* f = use_stderr? stderr: stdout;
        fprintf(f, "[%s] %s: %s\n", get_time(), priority_name, string);
    } else {
        syslog(priority, "[%s] %s", priority_name, string);
    }
}

void print_critical(const char* string) {
    print_priority(string, LOG_CRIT, true);
}

void print_error(const char* string) {
    print_priority(string, LOG_ERR, true);
}

void print_warning(const char* string) {
    print_priority(string, LOG_WARNING, false);
}

void print_info(const char* string) {
    print_priority(string, LOG_INFO, false);
}

void print_debug(const char* string) {
    print_priority(string, LOG_DEBUG, false);
}

void set_logging_limit(int priority) {
    log_limit = priority;
}