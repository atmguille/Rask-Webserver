#include "../srclib/socket/socket.h"
#include "../includes/thread_pool.h"
#include <unistd.h>

int main() {
    int port = 8080;
    int sockfd = socket_open(port, 10);
    ThreadPool* t_pool = t_pool_ini(sockfd);
    sleep(100);
    t_pool_destroy(t_pool);
}