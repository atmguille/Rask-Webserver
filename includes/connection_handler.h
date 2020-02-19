#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "../includes/config_parser.h"

#define MAX_BUFFER 8192
#define MAX_HEADERS 32

#define OK 0
#define ERROR -1
#define CLOSE_CONNECTION -2
#define PARSE_ERROR -3

typedef struct _Request Request;

/**
 * Handles a connection with the client, reading a request and answering with the requested data
 * This function won't close client_fd.
 * @param client_fd
 * @return OK, ERROR or CLOSE_CONNECTION. You should close client_fd
 */
int connection_handler(int client_fd, struct config *server_attrs);

#endif // CONNECTION_HANDLER_H
