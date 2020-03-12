#include "../include/response.h"
#include "../include/utils.h"
#include "../srclib/socket/socket.h"
#include "../srclib/dynamic_buffer/dynamic_buffer.h"
#include "../srclib/logging/logging.h"
#include "../srclib/execute_scripts/execute_scripts.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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
 */
void _add_common_headers(DynamicBuffer *db, struct config *server_attrs, int status_code, char *message) {
    char current_date[DATE_SIZE];
    char response_line[GENERAL_SIZE];

    // Get current date
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(current_date, sizeof(current_date), "%a, %d %b %Y %H:%M:%S %Z", &tm);

    sprintf(response_line, "HTTP/1.1 %d %s\r\n", status_code, message);

    dynamic_buffer_append_string(db, response_line);
    dynamic_buffer_append_string(db, "Date: ");
    dynamic_buffer_append_string(db, current_date);
    dynamic_buffer_append_string(db, "\r\nServer: ");
    dynamic_buffer_append_string(db, server_attrs->signature);
    dynamic_buffer_append_string(db, "\r\n");
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
    char body[GENERAL_SIZE];

    DynamicBuffer *db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        print_error("failed to allocate memory for dynamic buffer");
        return ERROR;
    }

    _add_common_headers(db, server_attrs,status_code, message);

    sprintf(body, "<!DOCTYPE html><h1>%s</h1>\r\n", message);

    dynamic_buffer_append_string(db, "Content-Type: text/html; charset=UTF-8\r\nContent-Length: ");
    dynamic_buffer_append_number(db, (ERROR_BODY_LEN + strlen(message)));
    dynamic_buffer_append_string(db, "\r\nConnection: close\r\n\r\n");
    dynamic_buffer_append_string(db, body);

    if (socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db)) < 0) {
        print_error("failed to send");
        dynamic_buffer_destroy(db);
        response_internal_server_error(client_fd, server_attrs);
        return ERROR;
    }

    return OK;
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

int response_internal_server_error(int client_fd, struct config *server_attrs) {
    return _response_error(client_fd, server_attrs, 500, "Internal Server Error");
}

/**
 * Gets filename from the path (if the path is "/", it will use the default one)
 * @param string with the path
 * @return a null-terminated filename that must be freed
 */
char *_get_filename(struct string path, struct config *server_attrs) {
    size_t base_path_length;
    char *filename;

    if (string_is_equal_to(path, "/")) {
        path.data = server_attrs->default_path;
        path.size = strlen(server_attrs->default_path);
    }

    base_path_length = strlen(server_attrs->base_path);
    filename = (char *)malloc(base_path_length + path.size * sizeof(char) + 1);
    if (filename == NULL) {
        print_error("failed to allocate memory for filename");
        return NULL;
    }

    strncpy(filename, server_attrs->base_path, base_path_length);
    strncpy(&filename[base_path_length], path.data, path.size);
    filename[path.size + base_path_length] = '\0';

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

/**
 * Returns content type from the specified extension
 * @param extension
 * @return content type
 * There's no memory allocation
 */
char *_get_content_type(const char *extension) {

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

/**
 * Builds and sends cgi response, executing the desired script
 * @param client_fd
 * @param server_attrs
 * @param filename
 * @param extension
 * @return status
 */
int _response_cgi(int client_fd, struct config *server_attrs, struct string args, char *filename, const char *extension) {
    DynamicBuffer *script_output;
    DynamicBuffer *response;

    response = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (response == NULL) {
        print_error("failed to allocate memory for dynamic buffer");
        response_internal_server_error(client_fd, server_attrs);
        return ERROR;
    }

    #ifdef __APPLE__
        _add_common_headers(response, server_attrs, 501, "Not Implemented");
        dynamic_buffer_append_string(response, "Content-Type: text/html; charset=UTF-8\r\n");
        dynamic_buffer_append_string(response, "Content-Length: 41\r\n");
        dynamic_buffer_append_string(response, "Connection: keep-alive\r\n\r\n");
        dynamic_buffer_append_string(response, "<h1>Scripts not implemented on macOS</h1>");
        socket_send(client_fd, dynamic_buffer_get_buffer(response), dynamic_buffer_get_size(response));
        return OK;
    #endif

    //Check if filename exists
    if (access(filename, F_OK) == -1) {
        response_not_found(client_fd, server_attrs);
        dynamic_buffer_destroy(response);
        return NOT_FOUND;
    }

    if (strcmp(extension, ".py") == 0) {
        script_output = execute_python_script(filename, args);
    } else if (strcmp(extension, ".php") == 0) {
        script_output = execute_php_script(filename, args);
    } else {
        response_bad_request(client_fd, server_attrs); // TODO: quizás otro código de error se adapte mejor
        dynamic_buffer_destroy(response);
        return BAD_REQUEST;
    }

    if (script_output == NULL) {
        response_internal_server_error(client_fd, server_attrs);
        dynamic_buffer_destroy(response);
        return ERROR;
    }

    _add_common_headers(response, server_attrs, 200, "OK");
    dynamic_buffer_append_string(response, "Content-Type: text/plain; charset=UTF-8\r\nContent-Length: ");
    dynamic_buffer_append_number(response, dynamic_buffer_get_size(script_output));
    dynamic_buffer_append_string(response, "\r\nConnection: keep-alive\r\n\r\n");
    dynamic_buffer_append(response, dynamic_buffer_get_buffer(script_output), dynamic_buffer_get_size(script_output));

    if (socket_send(client_fd, dynamic_buffer_get_buffer(response), dynamic_buffer_get_size(response)) < 0) {
        response_internal_server_error(client_fd, server_attrs);
        dynamic_buffer_destroy(response);
        dynamic_buffer_destroy(script_output);
        return ERROR;
    }

    dynamic_buffer_destroy(response);
    dynamic_buffer_destroy(script_output);
    return OK;
}


int response_get(int client_fd, struct config *server_attrs, struct request *request) {
    char *filename;
    const char *extension;
    char *content_type;
    size_t file_size;
    size_t bytes_read;
    long last_modified;
    char c_last_modified[GENERAL_SIZE]; // TODO: GENERAL SIZE???
    char etag[GENERAL_SIZE];
    FILE* f;
    DynamicBuffer *db;


    filename = _get_filename(request->path, server_attrs);
    if (filename == NULL) {
        response_internal_server_error(client_fd, server_attrs);
        return ERROR;
    }
    extension = _find_extension(filename);
    if (extension == NULL) {
        print_info("don't know what to do with directory %s", filename);
        response_not_implemented(client_fd, server_attrs); // TODO: not implemented or bad_request? en cgi devolvemos bad request y aqui esto
        free(filename);
        return ERROR;
    }

    // Check if extension is cgi type
    if (strcmp(extension, ".py") == 0 || strcmp(extension, ".php") == 0) {
        int cgi_ret = _response_cgi(client_fd, server_attrs, request->url_args, filename, extension);
        free(filename);
        return cgi_ret;
    }

    db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        print_error("failed to allocate memory for dynamic buffer");
        response_internal_server_error(client_fd, server_attrs);
        free(filename);
        return ERROR;
    }

    content_type = _get_content_type(extension);
    if (content_type == NULL) {
        print_error("unrecognized content type for %s", filename);
        response_not_found(client_fd, server_attrs);
        free(filename);
        dynamic_buffer_destroy(db);
        return ERROR;
    }

    print_debug("%s requested (type %s)", filename, content_type);
    f = fopen(filename, "r");
    if (f == NULL) {
        print_error("can't open %s: %s", filename, strerror(errno));
        response_not_found(client_fd, server_attrs);
        free(filename);
        dynamic_buffer_destroy(db);
        return ERROR;
    }
    file_size = _get_file_size(filename);

    last_modified = _get_file_last_modified(filename);
    strftime(c_last_modified, sizeof(c_last_modified), "Last modified: %a, %d %b %Y %H:%M:%S %Z\r\n", gmtime(&(last_modified)));

    struct string header;
    request_get_header(request, &header, "If-None-Match");

    snprintf(etag, GENERAL_SIZE, "%ld%.*s", last_modified, (int)request->path.size, request->path.data);

    if (string_is_equal_to(header, etag)) {
        _add_common_headers(db, server_attrs, 304, "Not Modified");
        dynamic_buffer_append_string(db, "\r\nConnection: keep-alive\r\n\r\n");
    } else {
        _add_common_headers(db, server_attrs, 200, "OK");
        dynamic_buffer_append_string(db, "Content-Type: ");
        dynamic_buffer_append_string(db, content_type);
        dynamic_buffer_append_string(db, "; charset=UTF-8\r\n");
        dynamic_buffer_append_string(db, "ETag: ");
        dynamic_buffer_append_string(db, etag);
        dynamic_buffer_append_string(db, "\r\nContent-Length: ");
        dynamic_buffer_append_number(db, file_size);
        dynamic_buffer_append_string(db, "\r\n");
        dynamic_buffer_append_string(db, c_last_modified);
        dynamic_buffer_append_string(db, "Connection: keep-alive\r\n\r\n");

        // Add the file chunked
        while (file_size > 0) {
            bytes_read = dynamic_buffer_append_file_chunked(db, f);
            file_size -= bytes_read;
            if (dynamic_buffer_is_full(db)) {
                socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db));
                dynamic_buffer_clear(db);
            }
        }
    }

    fclose(f);

    if (!dynamic_buffer_is_empty(db)) {
        socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db));
    }

    dynamic_buffer_destroy(db);
    free(filename);
    return OK;
}

int response_options(int client_fd, struct config *server_attrs) {
    DynamicBuffer *db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        print_error("failed to allocate memory for dynamic buffer");
        response_internal_server_error(client_fd, server_attrs);
        return ERROR;
    }

    _add_common_headers(db, server_attrs, 200, "OK");
    dynamic_buffer_append_string(db, "Allow: GET, POST, OPTIONS\r\n");
    dynamic_buffer_append_string(db, "Content-Length: 0\r\n");
    dynamic_buffer_append_string(db, "Connection: keep-alive\r\n\r\n");

    if (socket_send(client_fd, dynamic_buffer_get_buffer(db), dynamic_buffer_get_size(db)) < 0) {
        response_internal_server_error(client_fd, server_attrs);
        dynamic_buffer_destroy(db);
        return ERROR;
    }

    return OK;
}

int response_post(int client_fd, struct config *server_attrs, struct request *request) {
    char *filename;
    const char *extension;
    int cgi_ret;

    filename = _get_filename(request->path, server_attrs);
    if (filename == NULL) {
        response_internal_server_error(client_fd, server_attrs);
        return ERROR;
    }
    extension = _find_extension(filename);

    cgi_ret = _response_cgi(client_fd, server_attrs, request->body, filename, extension);
    free(filename);
    return cgi_ret;
}

