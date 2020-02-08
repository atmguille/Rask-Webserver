#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <strings.h>
#include <unistd.h>

/********
* FUNCTION: int open_tcp_socket(int server_port, char* ip, int max_pending_conections)
* ARGS_IN: int port - port to be assigned to the socket.
*          int max_pending_conections - maximum length to which the queue of pending connections for sockfd may grow        
* DESCRIPTION: a TCP socket with the port specified will be created, setting it ready to listen
* ARGS_OUT: int - socket file descriptor (EXIT_FAILURE if something went wrong)
********/
int open_tcp_socket(int port, int max_pending_conections);

/********
* FUNCTION: int accept_conexion(int sockfd)
* ARGS_IN: int sockfd - socket descriptor where the connection will be accepted.
* DESCRIPTION: a connection with a client will be accepted in the specified socket.
* ARGS_OUT: int - socket with connection with the client file descriptor (EXIT_FAILURE if something went wrong)
********/
int accept_connection(int sockfd);

/********
* FUNCTION: ssize_t send(int clientfd, const void *buf, size_t len)
* ARGS_IN: int clientfd - socket with connection with the client file descriptor
           const void *buf - buffer with the info to be sent
           size_t len - length of the message to be sent
* DESCRIPTION: send the message contained in buffer to the client of the connected socket
* ARGS_OUT: ssize_t number of bytes sent (EXIT_FAILURE if something went wrong)
********/
ssize_t my_send(int clientfd, const void *buf, size_t len);

/********
* FUNCTION: ssize_t recv(int sockfd, void *buf, size_t len)
* ARGS_IN: int clientfd - socket with connection with the client file descriptor
           void *buf - buffer to store the info received
           size_t len - request number of bytes
* DESCRIPTION: receive the message and store it in the buffer from the client of the connected socket
* ARGS_OUT: ssize_t number of bytes received (EXIT_FAILURE if something went wrong)
********/
ssize_t my_recv(int clientfd, void *buf, size_t len);

/********
* FUNCTION: void close_connection(int clientfd)
* ARGS_IN: int clientfd - socket with connection with the client file descriptor      
* DESCRIPTION: close the connection
* ARGS_OUT: void
********/
void close_connection(int clientfd);

/********
* FUNCTION: void close_socket(int sockfd)
* ARGS_IN: int sockfd - socket file descriptor       
* DESCRIPTION: close the socket 
* ARGS_OUT: void
********/
void close_socket(int sockfd);

#endif