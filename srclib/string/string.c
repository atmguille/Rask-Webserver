#include "string.h"


bool string_are_equal(struct string s1, struct string s2) {
    if (s1.size == s2.size) {
        for (int i = 0; i < s1.size; i++) {
            if (s1.data[i] != s2.data[i]) {
                return false;
            }
        }

        return true;
    } else {
        return false;
    }
}

bool string_is_equal_to(struct string s1, const char *s2) {
    int i;
    const char *iterator;

    for (i = 0, iterator = s2; i < s1.size && *iterator != '\0'; i++, iterator++) {
        if (s1.data[i] != *iterator) {
            return false;
        }
    }

    if (i == s1.size && *iterator == '\0') {
        return true;
    } else {
        return false;
    }
}