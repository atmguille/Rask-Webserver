#include "../includes/response.h"
#include "../srclib/socket/socket.h"
#include "../srclib/dynamic_buffer/dynamic_buffer.h"
#include "../srclib/logging/logging.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#define ERROR_BODY_LEN 24
#define DATE_SIZE 40
#define GENERAL_SIZE 100

#define MATCH(actual_extension, type) if (strcmp(extension, actual_extension) == 0) {return type;}


/**
 * Adds to dynamic buffer headers that are the same for all responses (server name, date, ...)
 * @param db dynamic buffer
 * @param server_attrs
 * @param status_code
 * @param message
 * @return
 */
int _add_common_headers(DynamicBuffer *db, struct config *server_attrs, int status_code, char *message) {
    char current_date[DATE_SIZE];
    char response_line[GENERAL_SIZE];

    // Get current date
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    if (strftime(current_date, sizeof(current_date), "%a, %d %b %Y %H:%M:%S %Z", &tm) == 0) {
        print_error("failed to get current date");
        return -1;
    }

    sprintf(response_line, "HTTP/1.1 %d %s\r\n", status_code, message);

    if (dynamic_buffer_append_string(db, response_line) == 0 ||
        dynamic_buffer_append_string(db, "Date: ") == 0 ||
        dynamic_buffer_append_string(db, current_date) == 0 ||
        dynamic_buffer_append_string(db, "\r\nServer: ") == 0 ||
        dynamic_buffer_append_string(db, server_attrs->signature) == 0 ||
        dynamic_buffer_append_string(db, "\r\n") == 0) {

        print_error("failed to add common headers because of dynamic buffer");
        return -1;
    }

    return 0;

}

/**
 * Builds and sends response with error code and message
 * @param client_fd
 * @param server_attrs
 * @param status_code
 * @param message
 * @return
 */
int _response_error(int client_fd, struct config *server_attrs, int status_code, char *message) {
    char content_length[GENERAL_SIZE];
    char body[GENERAL_SIZE];

    DynamicBuffer *db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        print_error("failed to allocate memory for dynamic buffer");
        return -1;
    }

    if (_add_common_headers(db, server_attrs,status_code, message) != 0) {
        dynamic_buffer_destroy(db);
        return -1;
    }

    sprintf(content_length, "%lu", ERROR_BODY_LEN + strlen(message));
    sprintf(body, "<!DOCTYPE html><h1>%s</h1>\r\n", message);

    if (dynamic_buffer_append_string(db, "Content-Type: text/html; charset=UTF-8\r\n"
                                     "Content-Length: ") == 0 ||
        dynamic_buffer_append_string(db, content_length) == 0 ||
        dynamic_buffer_append_string(db, "\r\nConnection: close\r\n\r\n") == 0 || // TODO: cerramos conexion seguro???
        dynamic_buffer_append_string(db, body) == 0) {

        print_error("failed to response error because of dynamic buffer");
        return -1;
    }

    return socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db));
}

int response_bad_request(int client_fd, struct config *server_attrs) {
    return _response_error(client_fd, server_attrs, 400, "Bad Request");
}

int response_request_too_long(int client_fd, struct config *server_attrs) {
    return _response_error(client_fd, server_attrs, 400, "Bad Request - Request Too Long");
}

int response_not_found(int client_fd, struct config *server_attrs) {
    return _response_error(client_fd, server_attrs, 404, "Not Found");
}

int response_not_implemented(int client_fd, struct config *server_attrs) {
    return _response_error(client_fd, server_attrs, 501, "Not Implemented");
}

/**
 * Gets filename from the path (if the path is "/", it will use the default one)
 * @param path non-null-terminated path
 * @param length of path
 * @return a null-terminated filename that must be freed
 */
char *_get_filename(const char *path, size_t length, struct config *server_attrs) {
    size_t base_path_length;
    char *filename;

    if (length == 1 && *path == '/') {
        path = server_attrs->default_path;
        length = strlen(server_attrs->default_path);
    }

    base_path_length = strlen(server_attrs->base_path);
    filename = (char *)malloc(base_path_length + length * sizeof(char) + 1);

    strncpy(filename, server_attrs->base_path, base_path_length);
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
const char *_find_extension(const char *filename) {
    const char *cursor;
    const char *extension = NULL;

    for (cursor = filename; *cursor != '\0'; cursor++) {
        if (*cursor == '.') {
            extension = cursor;
        }
    }

    return extension;
}

char *_get_content_type(const char *filename) {
    const char *extension = _find_extension(filename);

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
        return -1;
    } else {
        return st.st_size;
    }
}

/**
 * Gets last modified date from the file specified
 * @param filename
 * @return st_mtime or -1 if an error occurred
 */
long _get_file_last_modified(char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        print_error("couldn't provide %s: %s", filename, strerror(errno));
        return -1;
    } else {
        return st.st_mtime;
    }
}

int response_get(int client_fd, struct config *server_attrs, struct request *request) {
    char *filename;
    char *content_type;
    size_t file_size;
    char c_file_size[20]; // The maximum value of an unsigned long long is 18446744073709551615
    long last_modified;
    char c_last_modified[GENERAL_SIZE];
    FILE* f;

    DynamicBuffer *db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        print_error("failed to allocate memory for dynamic buffer");
        return ERROR;
    }


    filename = _get_filename(request->path, request->path_len, server_attrs);
    print_info("%s requested (type %s)", filename, _get_content_type(filename));
    content_type = _get_content_type(filename);
    if (content_type == NULL) {
        print_error("unrecognized content type for %s", filename);
        response_not_found(client_fd, server_attrs);
        free(filename);
        return ERROR;
    }
    f = fopen(filename, "r");
    if (f == NULL) {
        print_error("can't open %s: %s", filename, strerror(errno));
        response_not_found(client_fd, server_attrs);
        free(filename);
        return ERROR;
    }
    file_size = _get_file_size(filename);
    sprintf(c_file_size, "%zu", file_size);

    last_modified = _get_file_last_modified(filename);
    if (strftime(c_last_modified, sizeof(c_last_modified), "Last modified: %a, %d %b %Y %H:%M:%S %Z\r\n", gmtime(&(last_modified))) == 0) {
        print_error("failed to get last modified date");
        return ERROR;
    }

    _add_common_headers(db, server_attrs, 200, "OK");
    dynamic_buffer_append_string(db, _get_content_type(filename));
    dynamic_buffer_append_string(db, "; charset=UTF-8\r\n");
    dynamic_buffer_append_string(db, "Content-Length: ");
    dynamic_buffer_append_string(db, c_file_size);
    dynamic_buffer_append_string(db, "\r\n");
    dynamic_buffer_append_string(db, c_last_modified);
    dynamic_buffer_append_string(db, "Connection: keep-alive\r\n\r\n");
    dynamic_buffer_append_file(db, f, file_size);
    fclose(f);

    socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db));

    dynamic_buffer_destroy(db);
    free(filename);
    return OK;
}

int response_options(int client_fd, struct config *server_attrs) {
    DynamicBuffer *db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        print_error("failed to allocate memory for dynamic buffer");
        return ERROR;
    }

    _add_common_headers(db, server_attrs, 200, "OK");
    dynamic_buffer_append_string(db, "Allow: GET, POST, OPTIONS\r\n");
    dynamic_buffer_append_string(db, "Content-Length: 0\r\n");
    dynamic_buffer_append_string(db, "Connection: keep-alive\r\n\r\n");

    socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db));

    return OK;
}

int response_post(int client_fd, struct config *server_attrs, struct request *request) {
    struct phr_header *last_header;
    const char *body;
    int body_len;

    // Find the body from the last header
    last_header = &request->headers[request->num_headers - 1];
    // At the end of the header, "\r\n\r\n" is found, which has 4 characters.
    body = &last_header->value[last_header->value_len] + 4;
    body_len = (int) (request->len_buffer - (body - request->buffer));

    printf("==>%.*s", body_len, body);
    return OK;
}

