#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../includes/connection_handler.h"
#include "../srclib/picohttpparser/picohttpparser.h"
#include "../srclib/logging/logging.h"
#include "../srclib/socket/socket.h"
#include "../srclib/dynamic_buffer/dynamic_buffer.h"
#include "../includes/request.h"
#include "../includes/response.h"

enum http_method {
    GET,
    POST,
    OPTIONS,
    UNKNOWN
};

bool _is_method(struct request *request, char *method) {
    size_t method_len = strlen(method);
    return (method_len == request->method_len && strncmp(method, request->method, method_len) == 0);
}

enum http_method _get_method(struct request *request) {
    if (_is_method(request, "GET")) {
        return GET;
    } else if (_is_method(request, "POST")) {
        return POST;
    } else if (_is_method(request, "OPTIONS")) {
        return OPTIONS;
    } else {
        print_warning("method not implemented");
        return UNKNOWN;
    }
}

int connection_handler(int client_fd, struct config *server_attrs) {
    struct request *request;
    enum http_method method;
    int response_code;

    // Set client_fd socket timeout
    socket_set_timeout(client_fd, 10);

    request = request_ini();
    if (request == NULL) {
        return ERROR;
    }

    response_code = process_request(client_fd, request);
    if (response_code < 0) {
        if (response_code == BAD_REQUEST) {
            response_bad_request(client_fd, server_attrs);
        } else if (response_code == REQUEST_TOO_LONG) {
            response_request_too_long(client_fd, server_attrs);
        }
        free(request);
        return response_code;
    }

    method = _get_method(request);
    if (method == UNKNOWN) {
        response_not_implemented(client_fd, server_attrs);
        free(request);
        return ERROR;
    } else if (method == GET) {
        response_get(client_fd, server_attrs, request);
    } else if (method == POST) {
        response_post();
    } else if (method == OPTIONS) {
        response_options(client_fd, server_attrs);
    }

    free(request);

    return OK;
}