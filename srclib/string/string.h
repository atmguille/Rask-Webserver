#ifndef STRING_H
#define STRING_H

#include <stdio.h>
#include <stdbool.h>

struct string {
    const char *data;
    size_t size;
};

bool string_are_equal(struct string s1, struct string s2);

bool string_is_equal_to(struct string s1, const char *s2);

#endif // STRING_H
