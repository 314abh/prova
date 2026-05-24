#ifndef PROVA_STRING_H
#define PROVA_STRING_H

#include <stddef.h>

typedef struct PString {
    size_t length;
    size_t capacity;   /* maximum capacity */
    size_t start, end; /* last byte, ie index of null terminator */
    char *bytes; /* actual bytes */
} PString;

const PString *prova_make_string(const char *str);
const PString *prova_make_string_of_capacity(const char *str, size_t capacity);
const char *prova_string_view(const PString *p_str);
void prova_delete_string(PString *p_str);

#endif /* PROVA_STRING_H */
