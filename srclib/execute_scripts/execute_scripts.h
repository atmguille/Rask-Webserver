#ifndef EXECUTE_SCRIPTS_H
#define EXECUTE_SCRIPTS_H

#include "../string/string.h"
#include "../dynamic_buffer/dynamic_buffer.h"

/**
 * Executes python script located in path passing stdin_args through standard input until timeout expires
 * @param path
 * @param stdin_args
 * @param len_stdin_args
 * @param timeout
 * @return dynamic buffer with the script output
 */
DynamicBuffer *execute_python_script(char *path, struct string stdin_args, int timeout);

/**
 * Executes php script located in path passing stdin_args through standard input until timeout expires
 * @param path
 * @param stdin_args
 * @param len_stdin_args
 * @param timeout
 * @return dynamic buffer with the script output
 */
DynamicBuffer *execute_php_script(char *path, struct string stdin_args, int timeout);

#endif // EXECUTE_SCRIPTS_H
