#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>
#include "../srclib/picohttpparser/picohttpparser.h"
#include "../srclib/string/string.h"

#define MAX_BUFFER 8192
#define MAX_HEADERS 32

struct request {
    char buffer[MAX_BUFFER];
    size_t len_buffer;
    size_t old_len_buffer;
    struct string method;
    struct string path;
    int minor_version;
    struct phr_header headers[MAX_HEADERS];
    size_t num_headers;
    char *body;
    size_t body_len;
};

/**
 * Allocates memory and initializes values to macros
 * @return request
 */
struct request *request_ini();

/**
 * Read from the client_fd, parse the request and test if it's valid
 * @param client_fd
 * @param request
 * @return ERROR on error, 0 if everything was correct, CLOSE_CONNECTION if the client's fd should be closed
 */
int process_request(int client_fd, struct request *request);

#endif //REQUEST_H
