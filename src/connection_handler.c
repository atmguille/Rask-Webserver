#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "../includes/connection_handler.h"
#include "../srclib/picohttpparser/picohttpparser.h"
#include "../srclib/logging/logging.h"
#include "../srclib/socket/socket.h"
#include "../srclib/dynamic_buffer/dynamic_buffer.h"

#define MATCH(actual_extension, type) if (strcmp(extension, actual_extension) == 0) {return type;}

enum http_method {
    GET,
    POST,
    OPTIONS,
    UNKNOWN
};

struct _Request {
    char buffer[MAX_BUFFER];
    size_t len_buffer;
    size_t old_len_buffer;
    char *method;
    size_t method_len;
    char *path;
    size_t path_len;
    int minor_version;
    struct phr_header headers[MAX_HEADERS];
    size_t num_headers;
};

/**
 * Allocates memory and initializes values to macros
 * @return request
 */
Request *_request_ini() {
    Request *request;
    request = (Request *)malloc(sizeof(Request));
    if (request == NULL) {
        print_error("failed to allocate memory for request struct");
        return NULL;
    }
    request->len_buffer = 0;
    request ->num_headers = MAX_HEADERS;
    return request;
}

/**
 * Gets filename from the path (if the path is "/", it will use the default one)
 * @param path non-null-terminated path
 * @param length of path
 * @return a null-terminated filename that must be freed
 */
char *get_filename(const char *path, int length) {
    size_t base_path_length;
    char *filename;

    if (length == 1 && *path == '/') {
        path = DEFAULT_PATH;
        length = sizeof(DEFAULT_PATH) - 1;
    }

    base_path_length = sizeof(BASE_PATH) - 1;
    filename = (char *)malloc(base_path_length + length * sizeof(char) + 1);

    strncpy(filename, BASE_PATH, base_path_length);
    strncpy(&filename[base_path_length], path, length);
    filename[length + base_path_length] = '\0';

    return filename;
}

/**
 * Returns filename from the last dot ('.') on
 * @param filename
 * @return a pointer to the last dot of filename (or NULL if no '.' was found)
 * There's no memory allocation
 */
const char *find_extension(const char *filename) {
    const char *cursor;
    const char *extension = NULL;

    for (cursor = filename; *cursor != '\0'; cursor++) {
        if (*cursor == '.') {
            extension = cursor;
        }
    }

    return extension;
}

char *find_content_type(const char *filename) {
    const char* extension = find_extension(filename);

    MATCH(".txt", "text/plain")
    MATCH(".html", "text/html")
    MATCH(".htm", "text/html")
    MATCH(".gif", "image/gif")
    MATCH(".jpeg", "image/jpeg")
    MATCH(".png", "image/png")
    MATCH(".jpg", "image/jpeg")
    MATCH(".mpeg", "video/mpeg")
    MATCH(".mpg", "video/mpeg")
    MATCH(".mp4", "video/mp4")
    MATCH(".doc", "application/msword")
    MATCH(".docx", "application/msword")
    MATCH(".pdf", "application/pdf")

    return NULL;
}

/**
 * Gets the file's size in bytes
 * @param filename
 * @return size of filename in bytes or -1 if an error occurred
 */
size_t _get_file_size(char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        print_error("couldn't provide %s: %s", filename, strerror(errno));
        return 0;
    } else {
        return st.st_size;
    }
}

/**
 * Checks if a request is valid (method supported, ...)
 * @param request
 * @return true if valid
 */
bool _is_request_valid(Request *request) { // TODO: habrá que hacer más comprobaciones imagino
    return true;
}

/**
 * Read from the client_fd, parse the request and test if it's valid
 * @param client_fd
 * @param request
 * @return ERROR on error, 0 if everything was correct, CLOSE_CONNECTION if the client's fd should be closed
 */
int _process_request(int client_fd, Request *request) {
    ssize_t ret;

    while (true) {
        // Keep on reading if the read function was interrupted by a signal
        while ((ret = read(client_fd, &request->buffer[request->len_buffer], MAX_BUFFER - request->len_buffer)) == -1 &&
               errno == EINTR) {}
        if (ret < 0) {
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
            return ERROR;
        }
    }

    if (_is_request_valid(request)) {
        return OK;
    } else {
        socket_send_string(client_fd, "HTTP/1.1 400 Bad Request\r\n"
                                      "Content-Type: text/html; charset=UTF-8\r\n"
                                      "Content-Length: 33"
                                      "Connection: close\r\n\r\n"
                                      "<!DOCTYPE html><h1>Bad Request</h1>\r\n");
        return ERROR;
    }

}

bool _is_method(Request *request, char *method) {
    size_t method_len = strlen(method);
    return (method_len == request->method_len && strncmp(method, request->method, method_len) == 0);
}

enum http_method _get_method(Request *request) {
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

int connection_handler(int client_fd) {
    Request *request;
    enum http_method method;
    int response_code;

    char *filename;
    size_t file_size;
    char c_file_size[20]; // The maximum value of an unsigned long long is 18446744073709551615
    FILE* f;
    DynamicBuffer *db;

    request = _request_ini();
    if (request == NULL) {
        return ERROR;
    }

    response_code = _process_request(client_fd, request);
    if (response_code < 0) {
        free(request);
        return response_code;
    }

    method = _get_method(request);
    if (method == UNKNOWN) {
        socket_send_string(client_fd, "HTTP/1.1 501 Not Implemented\r\n"
                                      "Content-Type: text/html; charset=UTF-8\r\n"
                                      "Content-Length: 39"
                                      "Connection: close\r\n\r\n"
                                      "<!DOCTYPE html><h1>Not Implemented</h1>\r\n");
        free(request);
        return ERROR;
    }



    filename = get_filename(request->path, request->path_len);
    print_info("%s requested (type %s)", filename, find_content_type(filename));
    f = fopen(filename, "r");
    if (f == NULL) {
        print_error("can't open %s: %s", filename, strerror(errno));
        socket_send_string(client_fd, "HTTP/1.1 404 Not Found\r\n"
                                      "Content-Type: text/html; charset=UTF-8\r\n"
                                      "Content-Length: 33"
                                      "Connection: close\r\n\r\n"
                                      "<!DOCTYPE html><h1>Not Found</h1>\r\n");
        free(request);
        return ERROR;
    }
    file_size = _get_file_size(filename);
    sprintf(c_file_size, "%zu", file_size);

    // Build the response
    db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        // TODO: send error message
        free(request);
        return ERROR;
    }

    dynamic_buffer_append_string(db, "HTTP/1.1 200 Ok\r\nContent-Type: ");
    dynamic_buffer_append_string(db, find_content_type(filename));
    dynamic_buffer_append_string(db, "; charset=UTF-8\r\n");
    dynamic_buffer_append_string(db, "Connection: keep-alive\r\n");
    dynamic_buffer_append_string(db, "Content-Length: ");
    dynamic_buffer_append_string(db, c_file_size);
    dynamic_buffer_append_string(db, "\r\n\r\n");
    dynamic_buffer_append_file(db, f, file_size);
    fclose(f);

    socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db));

    dynamic_buffer_destroy(db);
    free(filename);
    free(request);

    return OK;
}