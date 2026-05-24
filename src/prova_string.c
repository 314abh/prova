#include "prova_string.h"
#include <stdlib.h>
#include <string.h>

const PString *prova_make_string(const char *str)
{
    size_t length = strlen(str) + 1;
    PString *p_str = (PString*) malloc(sizeof(*p_str) + length);
    p_str->length = length;
    p_str->capacity = length;
    p_str->start = 0;
    p_str->end = length - 1;
    p_str->bytes = memcpy(p_str->bytes, str, length);
    return p_str;

}
// PString *prova_make_string_of_capacity(const char *str, size_t capacity) {
//
// }
