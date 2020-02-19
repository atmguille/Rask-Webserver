#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#define DEFAULT_INITIAL_CAPACITY 4096

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
 * @return size or 0 if some error occurred
 */
size_t dynamic_buffer_append(DynamicBuffer *db, const void *src, size_t size);

/**
 * Appends string to db
 * @param db
 * @param string
 * @return size of 0 if some error occurred
 */
size_t dynamic_buffer_append_string(DynamicBuffer *db, const char *string);


/**
 * Appends size bytes of the file f to db
 * @param db
 * @param f file
 * @param size in bytes to be copied from f to db
 * @return size of 0 if an error occurred
 */
size_t dynamic_buffer_append_file(DynamicBuffer *db, FILE *f, size_t size);

/**
 * Gets the internal buffer
 * @param db
 * @return a buffer whose length in bytes is dynamic_buffer_get_size or NULL is db is NULL
 */
const void *dynamic_buffer_get_buffer(DynamicBuffer *db);

/**
 * Gets the internal buffer's size
 * @param db
 * @return the size (in bytes) o 0 if db is NULL
 */
size_t dynamic_buffer_get_size(DynamicBuffer *db);

/**
 * Destroys a dynamic_buffer
 * @param db dynamic_buffer to be destroyed
 */
void dynamic_buffer_destroy(DynamicBuffer *db);


#endif // DYNAMIC_BUFFER_H
