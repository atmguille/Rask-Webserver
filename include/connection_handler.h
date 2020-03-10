#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "../include/config_parser.h"
#include "../include/request.h" // TODO: esto es para que otros archivos que dependan del connection handler tengan los codigos de estado, quizas mejor algo comun

/**
 * Handles a connection with the client, reading a request and answering with the requested data
 * This function won't close client_fd.
 * @param client_fd
 * @return OK, ERROR or CLOSE_CONNECTION. You should close client_fd
 */
int connection_handler(int client_fd, struct config *server_attrs);

#endif // CONNECTION_HANDLER_H
