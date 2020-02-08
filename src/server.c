#include "lib/sockets/sockets.h"
#include "lib/thread_pool/thread_pool.h"

int main() {
    int port = 4444;
    int sockfd = open_tcp_socket(port, 10);
    ThreadPool* t_pool = t_pool_ini(sockfd);
    sleep(10);
    t_pool_destroy(t_pool);
}