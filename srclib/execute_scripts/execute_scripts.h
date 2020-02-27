#ifndef EXECUTE_SCRIPTS_H
#define EXECUTE_SCRIPTS_H

/**
 * Executes python script located in path passing stdin_args through standard input
 * @param path
 * @param stdin_args
 * @param len_stdin_args
 * @return string that must be freed with the script output
 */
char *execute_python_script(char *path, const char *stdin_args, int len_stdin_args);

/**
 * Executes php script located in path passing stdin_args through standard input
 * @param path
 * @param stdin_args
 * @param len_stdin_args
 * @return string that must be freed with the script output
 */
char *execute_php_script(char *path, const char *stdin_args, int len_stdin_args);

#endif //EXECUTE_SCRIPTS_H
