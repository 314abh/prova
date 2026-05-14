#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <ctype.h>

#include "calculator.h"

int addition(int x, int y) {
  return x + y;
}

int product(int x, int y) {
  return x * y;
}

float average(size_t n, ...) {
  float sum = 0.0f;

  va_list argptr;
  va_start(argptr, n);

  for (size_t i = 0; i < n; ++i)
    sum += (float) va_arg(argptr, double);

  va_end(argptr);
  return sum / (float) n;
}

char *to_uppercase(char *str) {
  if (str == NULL) return NULL;

  for (char *s = str; *s != '\0'; s++)
    *s = (char) toupper((unsigned char) *s);

  return str;
}

char *to_lowercase(char *str) {
  if (str == NULL) return NULL;

  for (char *s = str; *s != '\0'; s++)
    *s = (char) tolower((unsigned char) *s);

  return str;
}
