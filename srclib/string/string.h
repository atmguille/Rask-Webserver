#ifndef STRING_H
#define STRING_H

#include <stdio.h>
#include <stdbool.h>

struct string {
    const char *data;
    size_t size;
};

/**
 * Check if two struct strings are equal
 * @param s1
 * @param s2
 * @return true if both struct strings have the length and content, false otherwise
 */
bool string_are_equal(struct string s1, struct string s2);


/**
 * Check if a struct string and a regular null terminated string are equal
 * @param s1
 * @param s2 a null terminated string
 * @return true if both have the same length and content, false otherwise
 */
bool string_is_equal_to(struct string s1, const char *s2);

#endif // STRING_H
