#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "../include/request.h"
#include "../srclib/logging/logging.h"
#include "../include/utils.h"


/**
 * Checks if a request is valid (method supported, ...)
 * @param request
 * @return true if valid
 */
bool _is_request_valid(struct request *request) {
    for (unsigned int i = 0; i < request->path.size - 1; i++) {
        if (request->path.data[i] == '.' && request->path.data[i + 1] == '.') {
            return false;
        }
    }

    return true;
}

/**
 * Look for url_args after ? and fill the correct fields in the struct
 * @param request
 */
void _parse_url_args(struct request *request) {
    for (unsigned int i = 0; i < request->path.size; i++) {
        if (request->path.data[i] == '?') {
            request->url_args.data = &request->path.data[i + 1];
            request->url_args.size = request->path.size - i - 1;
            request->path.size = i;  // Update length so path ends just before ?
            return;
        }
    }
    // If there are no url_args
    request->url_args.data = NULL;
    request->url_args.size = 0;
}

/**
 * Fill the body fields in the struct with the data after the headers
 * @param request
 */
void _parse_body(struct request *request) {
    struct phr_header *last_header;

    // Find the body from the last header
    last_header = &request->headers[request->num_headers - 1];
    // At the end of the header, "\r\n\r\n" is found, which has 4 characters.
    request->body.data = (char *)&last_header->value[last_header->value_len] + 4;
    request->body.size = request->len_buffer - (request->body.data - request->buffer);
}

int request_process(struct request *request, int client_fd) {
    ssize_t ret;
    char *method;
    size_t method_len;
    char *path;
    size_t path_len;

    request->len_buffer = 0;
    request->num_headers = MAX_HEADERS;

    while (true) {
        // Keep on reading if the read function was interrupted by a signal
        while ((ret = read(client_fd, &request->buffer[request->len_buffer], MAX_BUFFER - request->len_buffer)) == -1 &&
               errno == EINTR) {}
        if (ret == ERROR && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            print_info("timeout %d (%d)", errno, ret);
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
                (const char **) &method,
                &method_len,
                (const char **) &path,
                &path_len,
                &request->minor_version,
                request->headers,
                &request->num_headers,
                request->old_len_buffer);
        if (ret > 0) {
            break;
        } else if (ret == -1) {
            print_error("error parsing request");
            return BAD_REQUEST;
        } else if (ret == -2 && request->len_buffer == MAX_BUFFER) {
            print_error("request is too long");
            return REQUEST_TOO_LONG;
        }
    }

    request->method.data = method;
    request->method.size = method_len;
    request->path.data = path;
    request->path.size = path_len;
    if (_is_request_valid(request)) {
        _parse_url_args(request);
        _parse_body(request);
        return OK;
    } else {
        return BAD_REQUEST;
    }

}


void request_get_header(struct request *request, struct string *header, const char *header_name) {
    for (unsigned int i = 0; i < request->num_headers; i++) {
        struct string current_header_name = {request->headers[i].name, request->headers[i].name_len};

        if (string_is_equal_to(current_header_name, header_name)) {
            header->data = request->headers[i].value;
            header->size = request->headers[i].value_len;
            return;
        }
    }

    header->data = NULL;
    header->size = 0;
}
