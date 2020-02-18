#include "../srclib/socket/socket.h"
#include "../includes/thread_pool.h"
#include "../srclib/picohttpparser/picohttpparser.h"
#include "../srclib/logging/logging.h"
#include "../includes/connection_handler.h"
#include "../srclib/dynamic_buffer/dynamic_buffer.h"
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>


int main() {
    int port = 8080;
    int socket_fd = socket_open(port, 10);
    int client_fd = socket_accept(socket_fd);

    while (true) {
        print_debug("Entrando en el handler...");
        if (connection_handler(client_fd) == -1) {
            socket_close(client_fd);
            client_fd = socket_accept(socket_fd);
        }
    }
}