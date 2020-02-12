#include "../srclib/socket/socket.h"
#include "../includes/thread_pool.h"
#include "../srclib/picohttpparser/picohttpparser.h"
#include "../srclib/logging/logging.h"
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_BUFFER 4096
#define MAX_HEADERS 100

int main() {
    int port = 8080;
    int socket_fd = socket_open(port, 10);
    int client_fd = socket_accept(socket_fd);

    ssize_t ret;
    size_t len_buffer = 0;
    size_t old_len_buffer;
    char buffer[MAX_BUFFER];
    struct phr_header headers[MAX_HEADERS];
    size_t num_headers = MAX_HEADERS;
    char *method;
    size_t method_len;
    char *path;
    size_t path_len;
    int minor_version;

    while (true) {
        // Keep on reading if the read function was interrupted by a signal
        while ((ret = read(client_fd, &buffer[len_buffer], MAX_BUFFER - len_buffer)) == -1 && errno == EINTR) {}
        if (ret <= 0) {
            return EXIT_FAILURE;
        }

        old_len_buffer = len_buffer;
        len_buffer += ret;

        // Parse the request
        ret = phr_parse_request(
                buffer,
                len_buffer,
                (const char **) &method,
                &method_len,
                (const char **) &path,
                &path_len,
                &minor_version,
                headers,
                &num_headers,
                old_len_buffer);

        if (ret > 0) {
            break;
        } else if (ret == -1) {
            print_error("error parsing request");
            return EXIT_FAILURE;
        } else if (ret == -2 && len_buffer == MAX_BUFFER) {
            print_error("request is too long");
            return EXIT_FAILURE;
        }
    }

    printf("request is %zd bytes long\n", ret);
    printf("method is %.*s\n", (int)method_len, method);
    printf("path is %.*s\n", (int)path_len, path);
    printf("HTTP version is 1.%d\n", minor_version);
    printf("headers:\n");
    for (int i = 0; i != num_headers; ++i) {
        printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
               (int)headers[i].value_len, headers[i].value);
    }

    socket_send_string(client_fd, "HTTP/1.1 200 Ok\r\n"
                                  "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                                  "<!DOCTYPE html>\r\n"
                                  "<html><head><title>Servidor Web en C</title>\r\n"
                                  "<style>body { background-color: #AFD0F5 }</style></head>\r\n"
                                  "<body><center><h1>Hola mundo!</h1><br>\r\n");
    socket_close(socket_fd);

}