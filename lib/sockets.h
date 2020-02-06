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
*          char* ip - array of chars containing the desired IP to be assigned to the socket. 
*          int max_pending_conections - maximum length to which the queue of pending connections for sockfd may grow        
* DESCRIPTION: a TCP socket with the port and ip specified will be created, setting it ready to listen
* ARGS_OUT: int - socket file descriptor
********/
int open_tcp_socket(int port, char* ip, int max_pending_conections);

/********
* FUNCTION: void close_socket(int sockfd)
* ARGS_IN: int sockfd - socket file descriptor       
* DESCRIPTION: close the socket 
* ARGS_OUT: void
********/
void close_socket(int sockfd);

#endif