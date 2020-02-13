#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#define DEFAULT_INITIAL_CAPACITY 4096

typedef struct _DynamicBuffer DynamicBuffer;

/**
 * Creates a DynamicBuffer
 * @param initial_capacity of the internal buffer. If it is < 1, it is set to DEFAULT_INITIAL_CAPACITY
 * @return the DynamicBuffer or NULL if some error occurred
 */
DynamicBuffer *dynamic_buffer_ini(int initial_capacity);

/**
 * Destroys a DynamicBuffer
 * @param db DynamicBuffer to be destroyed
 */
void dynamic_buffer_destroy(DynamicBuffer *db);

size_t dynamic_buffer_append(DynamicBuffer *db, const void *src, size_t size);

size_t dynamic_buffer_append_string(DynamicBuffer *db, const char *string);

const void *dynamic_buffer_get_buffer(DynamicBuffer *db);

size_t dynamic_buffer_get_size(DynamicBuffer *db);

#endif // DYNAMIC_BUFFER_H
