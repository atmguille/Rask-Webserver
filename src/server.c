#include "../srclib/socket/socket.h"
#include "../includes/thread_pool.h"
#include "../includes/config_parser.h"
#include <unistd.h>

int main() {
    ServerAttributes    *serverAttributes;
    int                  socked_fd;
    ThreadPool          *threadPool;

    serverAttributes = server_attr_load("../files/server.conf");
    if (serverAttributes == NULL) {
        return 1;
    }
    socked_fd = socket_open(serverAttributes->listen_port, 10);
    if (socked_fd < 0) {
        server_attr_destroy(serverAttributes);
        return 1;
    }

    threadPool = thread_pool_ini(socked_fd, 8);
    if (threadPool == NULL) {
        server_attr_destroy(serverAttributes);
        return 1;
    }
    sleep(1);
    thread_pool_destroy(threadPool);
    server_attr_destroy(serverAttributes);
}