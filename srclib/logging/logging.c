#include <syslog.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

//  LOG_ERR is 1 and LOG_DEBUG is 7, so a smaller numbers means more important logs
int log_limit = LOG_DEBUG;

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

void print_priority(int priority, bool use_stderr, const char* format, va_list args) {
    const char* priority_name;
    FILE* f;

    // Don't log messages less important than the limit
    if (priority > log_limit) {
        return;
    }

    priority_name = get_priority_name(priority);
    f = use_stderr? stderr: stdout;

    // If the process was run with systemd
    if (getenv("RUN_FROM_SYSTEMD")) {
        fprintf(f, "%s: ", priority_name);
    } else {
        fprintf(f, "[%s] %s: ", get_time(), priority_name);
    }
    vfprintf(f, format, args);
    fprintf(f, "\n");
}

void print_critical(const char* format, ...) {
    va_list args;
    va_start(args, format);
    print_priority(LOG_CRIT, true, format, args);
}

void print_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    print_priority(LOG_ERR, true, format, args);
}

void print_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    print_priority(LOG_WARNING, false, format, args);
}

void print_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    print_priority(LOG_INFO, false, format, args);
}

void print_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    print_priority(LOG_DEBUG, false, format, args);
}

void set_logging_limit(int priority) {
    log_limit = priority;
}