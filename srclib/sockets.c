#include "lib/sockets.h"

int open_tcp_socket(int port, int max_pending_conections) {
    int sockfd;
    struct sockaddr_in myaddr;

    if (port < 0 || max_pending_conections < 0) {
        perror("Arguments must be positive");
        return EXIT_FAILURE;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error when creating the socket");
        return EXIT_FAILURE;
    }

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) != 0) {
        perror("socket--bind");
        return EXIT_FAILURE;
    }

    if (listen(sockfd, max_pending_conections) != 0) {
        perror("socket--listen");
        return EXIT_FAILURE;
    }

    return sockfd;
}

int accept_connection(int sockfd) {
    int clientfd;

    if (sockfd < 0) {
        perror("Bad socket descriptor when accepting conexion");
        return EXIT_FAILURE;
    }

    if ((clientfd = accept(sockfd, NULL, NULL)) < 0) { // Information from the client is not needed
        perror("Error accepting connection");
        return EXIT_FAILURE;
    }

    return clientfd;
}

ssize_t my_send(int clientfd, const void *buf, size_t len) {
    ssize_t bytes_sent;

    if (buf == NULL) {
        perror("Buffer is NULL when sending message");
        return EXIT_FAILURE;
    }

    if ((bytes_sent = send(clientfd, buf, len, 0)) < 0) { // We are not going to use FLAGS
        perror("No bytes sent");
        return EXIT_FAILURE;
    }

    syslog(LOG_DEBUG, "%d bytes sent", bytes_sent);

    return bytes_sent;
}

ssize_t my_recv(int clientfd, void *buf, size_t len) {
    ssize_t bytes_received;

    if ((bytes_received = recv(clientfd, buf, len, 0)) < 0) { // We are not going to use FLAGS
        perror("No bytes received");
        return EXIT_FAILURE;
    }

    syslog(LOG_DEBUG, "%d bytes received", bytes_received);

    return bytes_received;
}

void close_connection(int clientfd) {
    close(clientfd)
}

void close_socket(int sockfd) {
    close(sockfd);
}