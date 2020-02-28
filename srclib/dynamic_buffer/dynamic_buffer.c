#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "dynamic_buffer.h"
#include "../logging/logging.h"

struct _DynamicBuffer {
    size_t capacity;
    size_t size;
    void *buffer;
};

DynamicBuffer *dynamic_buffer_ini(int initial_capacity) {
    DynamicBuffer *sb;

    if (initial_capacity < 1) {
        print_warning("initial capacity cannot be %d, setting it to %d", initial_capacity, DEFAULT_INITIAL_CAPACITY);
        initial_capacity = DEFAULT_INITIAL_CAPACITY;
    }

    sb = (DynamicBuffer *)malloc(sizeof(*sb));
    if (sb == NULL) {
        print_error("failed to allocate memory");
        return NULL;
    }

    sb->capacity = initial_capacity;
    sb->buffer = (char *)malloc(sb->capacity);
    if (sb->buffer == NULL) {
        print_error("failed to allocate memory");
        return  NULL;
    }

    sb->size = 0;

    return sb;
}

void _grow_buffer(DynamicBuffer *db, size_t size) {
    if (db->size + size > db->capacity) {
        void *new_buffer = realloc(db->buffer, db->size + size);
        if (new_buffer == NULL) {
            print_error("failed to allocate memory");
        } else {
            print_debug("allocated more memory, from %zu to %zu", db->capacity, db->size + size);
            db->buffer = new_buffer;
            db->capacity = db->size + size;
        }
    }
}

bool _can_fit(DynamicBuffer *db, size_t size) {
    return size <= (db->capacity - db->size);
}

size_t dynamic_buffer_append(DynamicBuffer *db, const void *src, size_t size) {
    if (db == NULL || src == NULL) {
        print_warning("NULL passed to dynamic_buffer_append");
        return 0;
    }

    _grow_buffer(db, size);
    // If _grow_buffer failed there won't be enough space for memcpy
    if (!_can_fit(db, size)) {
        return 0;
    }

    memcpy(&db->buffer[db->size], src, size);
    db->size += size;

    return size;
}

size_t dynamic_buffer_append_string(DynamicBuffer *db, const char *string) {
    return dynamic_buffer_append(db, (const void *) string, strlen(string));
}

size_t dynamic_buffer_append_file(DynamicBuffer *db, FILE *f, size_t size) {
    size_t bytes_read;
    if (db == NULL || f == NULL) {
        print_warning("NULL passed to dynamic_buffer_append_file");
        return 0;
    }

    _grow_buffer(db, size);
    // If _grow_buffer failed there won't be enough space for memcpy
    if (!_can_fit(db, size)) {
        return 0;
    }

    bytes_read = fread(&db->buffer[db->size], sizeof(char), size, f);
    db->size += bytes_read;
    return bytes_read;
}

size_t dynamic_buffer_append_file_chunked(DynamicBuffer *db, FILE *f) {
    size_t bytes_read;
    size_t available_space;

    if (db == NULL || f == NULL) {
        print_warning("NULL passed to dynamic_buffer_append_file_chunked");
        return 0;
    }

    available_space = db->capacity - db->size;
    bytes_read = fread(&db->buffer[db->size], sizeof(char), available_space, f);
    db->size += bytes_read;

    return bytes_read;
}

bool dynamic_buffer_is_empty(DynamicBuffer *db) {
    return db != NULL && db->size == 0;
}

bool dynamic_buffer_is_full(DynamicBuffer *db) {
    return db != NULL && db->size == db->capacity;
}

void dynamic_buffer_clear(DynamicBuffer *db) {
    if (db != NULL) {
        db->size = 0;
    }
}

const void *dynamic_buffer_get_buffer(DynamicBuffer *db) {
    if (db == NULL) {
        return NULL;
    }

    return db->buffer;
}

size_t dynamic_buffer_get_size(DynamicBuffer *db) {
    if (db == NULL) {
        return 0;
    }

    return db->size;
}

void dynamic_buffer_destroy(DynamicBuffer *db) {
    if (db != NULL) {
        free(db->buffer);
        free(db);
    }
}
