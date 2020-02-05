#include "lib/sockets.h"

int open_tcp_socket(int port, char* ip, int max_pending_conections) {
    int sockfd;
    struct sockaddr_in myaddr;

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("Error when creating the socket");
        return EXIT_FAILURE;
    }

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    inet_aton(ip, &myaddr.sin_addr);

    if ( bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) != 0 ) {
        perror("socket--bind");
        return EXIT_FAILURE;
    }

    if ( listen(sockfd, max_pending_conections) != 0 ) {
        perror("socket--listen");
        return EXIT_FAILURE;
    }

    return sockfd;
}

void close_socket(int sockfd) {
    close(sockfd);
}