#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#define MAX_BUFFER 4096
#define MAX_HEADERS 32
#define BASE_PATH "../www"

/**
 * Handles a connection with the client, reading a request and answering with the requested data
 * This function won't close client_fd.
 * @param client_fd
 * @return 0 if no errors occurred and -1 otherwise
 */
int connection_handler(int client_fd);

#endif // CONNECTION_HANDLER_H
