#ifndef LOGGING_H
#define LOGGING_H

/**
 * Prints a critical message to stderr or to the system log if the process is a daemon
 * @param format of the message that will be printed
 */
void print_critical(const char* string, ...);

/**
 * Prints an error message to stderr or to the system log if the process is a daemon
 * @param format of the message that will be printed
 */
void print_error(const char* format, ...);

/**
 * Prints a warning message to stdout or to the system log if the process is a daemon
 * @param format of the message that will be printed
 */
void print_warning(const char* format, ...);

/**
 * Prints an info message to stdout or to the system log if the process is a daemon
 * @param format of the message that will be printed
 */
void print_info(const char* format, ...);

/**
 * Prints a debug message to stdout or to the system log if the process is a daemon
 * @param format of the message that will be printed
 */
void print_debug(const char* format, ...);

/**
 * Sets the logging limit, so the print_ functions will only work if the priority is higher than the threshold set here
 * The priorities valid are set in <syslog.h>, and they go from 1 (LOG_CRIT) to 7 (LOG_DEBUG)
 * @param priority
 */
void set_logging_limit(int priority);

#endif // LOGGING_H
