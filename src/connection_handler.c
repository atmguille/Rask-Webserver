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
size_t get_file_size(char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        print_error("couldn't provide %s: %s", filename, strerror(errno));
        return 0;
    } else {
        return st.st_size;
    }
}

int connection_handler(int client_fd) {
    char buffer[MAX_BUFFER];
    size_t len_buffer = 0;
    size_t old_len_buffer;
    ssize_t ret;
    struct phr_header headers[MAX_HEADERS];
    size_t num_headers = MAX_HEADERS;
    char *method;
    size_t method_len;
    char *path;
    size_t path_len;
    int minor_version;

    char *filename;
    size_t file_size;
    char c_file_size[20]; // The maximum value of an unsigned long long is 18446744073709551615
    FILE* f;
    DynamicBuffer *db;

    while (true) {
        // Keep on reading if the read function was interrupted by a signal
        while ((ret = read(client_fd, &buffer[len_buffer], MAX_BUFFER - len_buffer)) == -1 && errno == EINTR) {}
        if (ret < 0) {
            print_error("failed to read from client: %s", strerror(errno));
            return -1;
        } else if (ret == 0) {
            print_info("client has disconnected");
            return -1;
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
            return -1;
        } else if (ret == -2 && len_buffer == MAX_BUFFER) {
            print_error("request is too long");
            return -1;
        }
    }

    filename = get_filename(path, path_len);
    print_info("%s requested (type %s)", filename, find_content_type(filename));
    f = fopen(filename, "r");
    if (f == NULL) {
        print_error("can't open %s: %s", filename, strerror(errno));
        socket_send_string(client_fd, "HTTP/1.1 404 Not Found\r\n"
                                      "Content-Type: text/html; charset=UTF-8\r\n"
                                      "Content-Length: 33"
                                      "Connection: close\r\n\r\n"
                                      "<!DOCTYPE html><h1>Not Found</h1>\r\n");
        return -1;
    }
    file_size = get_file_size(filename);
    sprintf(c_file_size, "%zu", file_size);

    // Build the response
    db = (DynamicBuffer *)dynamic_buffer_ini(DEFAULT_INITIAL_CAPACITY);
    if (db == NULL) {
        // TODO: send error message
        return -1;
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

    return 0;
}