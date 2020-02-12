#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "../srclib/socket/socket.h"

int socket_fd = -1;

void signal_handler(int sig) {
    socket_close(socket_fd);
    exit(EXIT_SUCCESS);
}

int main() {
    socket_fd = socket_open(80, 125);

    if (socket_fd == ERROR) {
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    while (true) {
        int connection = socket_accept(socket_fd);
        socket_send_string(connection, "<h1>Bienvenidos a mi p&aacute;gina web</h1>\n<p>Hecha por Daniel Gallo y Guillermo Garc&iacute;a Cobo</p>\n");
        socket_close(connection);
    }
}