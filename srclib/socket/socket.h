#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdio.h>

#define ERROR -1

/**
 * Opens an active socket ready to accept connections. This function calls socket, bind and listen.
 * @param port to be assigned to the socket
 * @param max_pending_connections maximum length to which the queue of pending connections for the socket may grow
 * @return the socket's file descriptor or ERROR if something went wrong
 */
int socket_open(int port, int max_pending_connections);

/**
 * Accepts a connection to the socket referred by socket_fd
 * @param socket_fd file descriptor of an active socket (for example the socket_fd that socket_open returns)
 * @return the client's socket's file descriptor or ERROR if something went wrong
 */
int socket_accept(int socket_fd);

/**
 * Sends data through the socket referred by client_fd
 * @param client_fd client's socket's file descriptor
 * @param buffer buffer with the data to be sent
 * @param len length of the message contained in the buffer
 * @return the number of bytes sent or ERROR if something went wrong
 */
ssize_t socket_send(int client_fd, const void* buffer, size_t len);

/**
 * Sends a string through the socket referred by client_fd
 * @param client_fd client's socket's file descriptor
 * @param string string to be sent
 * @return the number of bytes sent or ERROR if something went wrong
 */
ssize_t socket_send_string(int client_fd, const char* string);

/**
 * Reads data from the socket referred by client_fd
 * @param client_fd client's socket's file descriptor
 * @param buffer buffer in which the incoming data will be stored
 * @param len length of the data to be read
 * @return number of bytes received or error if something went wrong
 */
ssize_t socket_receive(int client_fd, void *buffer, size_t len);

/**
 * Closes a file descriptor
 * @param socket_fd a file descriptor
 */
void socket_close(int socket_fd);

#endif // SOCKET_H