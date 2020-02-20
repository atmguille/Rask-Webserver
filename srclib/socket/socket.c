#include "socket.h"
#include "../logging/logging.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <strings.h>
#include <unistd.h>

char* ip_to_string(struct sockaddr* addr) {
    char* s;

    switch (addr->sa_family) {
        case AF_INET: {
            struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
            s = malloc(INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)addr;
            s = malloc(INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
            break;
        }
        default: {
            s = malloc(sizeof(char));
            *s = '\0';
            break;
        }
    }

    return s;
}

int socket_open(int port, int max_pending_connections) {
    int socket_fd;
    struct sockaddr_in addr;

    if (port < 0 || max_pending_connections < 0) {
        print_error("socket_open arguments must be positive");
        return ERROR;
    }

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        print_error("failed to create socket: %s", strerror(errno));
        return ERROR;
    }

    // Enable SO_REUSEADDR
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == ERROR) {
        print_error("failed to set socket's options: %s", strerror(errno));
        return ERROR;
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (const struct sockaddr*)&addr, sizeof(addr)) == ERROR) {
        print_error("failed to bind socket: %s", strerror(errno));
        return ERROR;
    }

    if (listen(socket_fd, max_pending_connections) == ERROR) {
        print_error("error setting socket as a passive one: %s", strerror(errno));
        return ERROR;
    }

    return socket_fd;
}

int socket_accept(int socket_fd) {
    int client_fd;
    char* client_ip;
    struct sockaddr addr;
    socklen_t addr_len = sizeof(addr);

    if (socket_fd < 0) {
        print_error("negative socket_fd passed to socket_accept");
        return ERROR;
    }

    if ((client_fd = accept(socket_fd, &addr, &addr_len)) < 0) {
        print_error("error accepting connection: %s", strerror(errno));
        return ERROR;
    }

    client_ip = ip_to_string(&addr);
    print_info("connection accepted from %s", client_ip);
    free(client_ip);

    return client_fd;
}

ssize_t socket_send(int client_fd, const void* buffer, size_t len) {
    ssize_t bytes_sent;

    if (buffer == NULL) {
        print_error("buffer is null in socket_send");
        return ERROR;
    }

    if ((bytes_sent = send(client_fd, buffer, len, 0)) == ERROR) { // We are not going to use FLAGS
        print_error("error sending data through the socket: %s", strerror(errno));
        return ERROR;
    }

    print_debug("%d bytes sent", bytes_sent);

    return bytes_sent;
}

ssize_t socket_send_string(int client_fd, const char* string) {
    size_t len = strlen(string) * sizeof(char);

    return socket_send(client_fd, (const void*)string, len);
}

ssize_t socket_receive(int client_fd, void* buffer, size_t len) {
    ssize_t bytes_received;

    if ((bytes_received = recv(client_fd, buffer, len, 0)) == ERROR) { // We are not going to use FLAGS
        print_error("error receiving data from the socket: %s", strerror(errno));
        return ERROR;
    }

    print_debug("%d bytes received", bytes_received);

    return bytes_received;
}

int socket_set_timeout(int client_fd, unsigned int timeout) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) != 0) {
        print_error("error setting timeout for the socket: %s", strerror(errno));
        return ERROR;
    }

    return 0;
}

void socket_close(int socket_fd) {
    close(socket_fd);
}
