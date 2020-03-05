#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "url_encoded_form_parser.h"

// TODO: que hacemos con estos maravillosos archivos????

struct _Form {
    const char **keys;
    const char **values;
    int *key_sizes;
    int *value_sizes;
    int n;
};

Form *form_ini(const char *body, int body_size) {
    Form *form;
    int i;
    int n = 1; // TODO: comprobar que nos pasen 0 argumentos
    int current_index = 0;
    int current_size = 0;
    bool parsing_key = true;

    // Get the number of pairs (key, value)
    for (i = 0; i < body_size; i++) {
        if (body[i] == '&') {
            n++;
        }
    }

    form = (Form *)malloc(sizeof(*form));
    if (form == NULL) {
        return NULL;
    }

    form->n = n;
    form->keys = (char **)malloc(n * sizeof(*form->keys));
    if (form->keys == NULL) {
        free(form);
        return NULL;
    }
    form->values = (char **)malloc(n * sizeof(*form->values));
    if (form->values == NULL) {
        free(form->keys);
        free(form);
        return NULL;
    }
    form->key_sizes = (int *)malloc(n * sizeof(*form->key_sizes));
    if (form->key_sizes == NULL) {
        free(form->keys);
        free(form->values);
        free(form);
        return NULL;
    }
    form->value_sizes = (int *)malloc(n * sizeof(*form->value_sizes));
    if (form->key_sizes == NULL) {
        free(form->keys);
        free(form->values);
        free(form->key_sizes);
        free(form);
        return NULL;
    }

    form->keys[0] = body;
    for (i = 0; i < body_size; i++) {
        if (body[i] == '=') {
            form->key_sizes[current_index] = current_size;
            // TODO: si está vacío??
            form->values[current_index] = &body[i + 1];
            if (current_index == n - 1) {
                form->value_sizes[current_index] = body_size - i - 1;
                break;
            }
            current_size = 0;
        } else if (body[i] == '&') {
            form->value_sizes[current_index] = current_size;
            form->keys[++current_index] = &body[i + 1];
            current_size = 0;
        } else {
            current_size++;
        }
    }

    return form;
}

void form_print(Form *form) {
    if (form == NULL) {
        return;
    }

    for (int i = 0; i < form->n; i++) {
        printf("%.*s: %.*s\n", form->key_sizes[i], form->keys[i], form->value_sizes[i], form->values[i]);
    }
}