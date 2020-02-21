#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zconf.h>
#include <errno.h>
#include <string.h>
#include "../includes/request.h"
#include "../srclib/logging/logging.h"

struct request *request_ini() {
    struct request *request;
    request = (struct request *)malloc(sizeof(struct request));
    if (request == NULL) {
        print_error("failed to allocate memory for request struct");
        return NULL;
    }
    request->len_buffer = 0;
    request->num_headers = MAX_HEADERS;
    return request;
}

/**
 * Checks if a request is valid (method supported, ...)
 * @param request
 * @return true if valid
 */
bool _is_request_valid(struct request *request) { // TODO: habrá que hacer más comprobaciones imagino
    return true;
}

int process_request(int client_fd, struct request *request) {
    ssize_t ret;

    while (true) {
        // Keep on reading if the read function was interrupted by a signal
        while ((ret = read(client_fd, &request->buffer[request->len_buffer], MAX_BUFFER - request->len_buffer)) == -1 &&
               errno == EINTR) {}
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            print_info("timeout");
            return CLOSE_CONNECTION;
        } else if (ret < 0) {
            print_error("failed to read from client: %s", strerror(errno));
            return ERROR;
        } else if (ret == 0) {
            print_info("client has disconnected");
            return CLOSE_CONNECTION;
        }

        request->old_len_buffer = request->len_buffer;
        request->len_buffer += ret;

        // Parse the request
        ret = phr_parse_request(
                request->buffer,
                request->len_buffer,
                (const char **) &request->method,
                &request->method_len,
                (const char **) &request->path,
                &request->path_len,
                &request->minor_version,
                request->headers,
                &request->num_headers,
                request->old_len_buffer);
        if (ret > 0) {
            break;
        } else if (ret == -1) {
            print_error("error parsing request");
            return PARSE_ERROR;
        } else if (ret == -2 && request->len_buffer == MAX_BUFFER) {
            print_error("request is too long"); // TODO: handle this case
            return REQUEST_TOO_LONG;
        }
    }

    if (_is_request_valid(request)) {
        return OK;
    } else {
        return BAD_REQUEST;
    }

}

