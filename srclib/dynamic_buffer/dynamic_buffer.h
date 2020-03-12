#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define DEFAULT_INITIAL_CAPACITY 4096
#define DEFAULT_FD_BUFFER 256

typedef struct _DynamicBuffer DynamicBuffer;


/**
 * Creates a dynamic_buffer
 * @param initial_capacity of the internal buffer. If it is < 1, it is set to DEFAULT_INITIAL_CAPACITY
 * @return the dynamic_buffer or NULL if some error occurred. The dynamic_buffer must be freed using
 * dynamic_buffer_destroy
 */
DynamicBuffer *dynamic_buffer_ini(int initial_capacity);

/**
 * Appends src to db
 * @param db
 * @param src
 * @param size number of bytes to be copied from src to db
 * @return number of bytes read (size if no errors occurred)
 */
size_t dynamic_buffer_append(DynamicBuffer *db, const void *src, size_t size);

/**
 * Appends string to db
 * @param db
 * @param string
 * @return number of bytes read (strlen(string) if no errors occurred)
 */
size_t dynamic_buffer_append_string(DynamicBuffer *db, const char *string);

/**
 * Appends a size_t number
 * @param db
 * @param n size_t number
 * @return number of bytes written to the buffer
 */
size_t dynamic_buffer_append_number(DynamicBuffer *db, size_t n);

/**
 * Appends size bytes of the file f to db
 * @param db
 * @param f file
 * @param size in bytes to be copied from f to db
 * @return number of bytes read (size if no errors occurred)
 */
size_t dynamic_buffer_append_file(DynamicBuffer *db, FILE *f, size_t size);


/**
 * Appends the data received by fd to db (unless timeout expires)
 * @param db
 * @param fd file descriptor
 * @param timeout
 * @return size of the data received (0 if error or timeout)
 */
size_t dynamic_buffer_append_fd_with_timeout(DynamicBuffer *db, int fd, int timeout);

/**
 * Appends the contents of f to the internal buffer WITHOUT resizing it
 * @param db
 * @param f
 * @return the number of bytes read
 */
size_t dynamic_buffer_append_file_chunked(DynamicBuffer *db, FILE *f);

/**
 * @param db
 * @return true if db is not null and emtpy
 */
bool dynamic_buffer_is_empty(DynamicBuffer *db);

/**
 * @param db
 * @return true if db is not null and full
 */
bool dynamic_buffer_is_full(DynamicBuffer *db);

/**
 * Clears the dynamic buffer db
 * @param db
 */
void dynamic_buffer_clear(DynamicBuffer *db);

/**
 * Gets the internal buffer
 * @param db
 * @return a buffer whose length in bytes is dynamic_buffer_get_size or NULL is db is NULL
 */
const void *dynamic_buffer_get_buffer(DynamicBuffer *db);

/**
 * Gets the internal buffer's size
 * @param db
 * @return the size of the internal buffer (in bytes) or 0 if db is NULL
 */
size_t dynamic_buffer_get_size(DynamicBuffer *db);

/**
 * Destroys a dynamic_buffer
 * @param db dynamic_buffer to be destroyed
 */
void dynamic_buffer_destroy(DynamicBuffer *db);


#endif // DYNAMIC_BUFFER_H
