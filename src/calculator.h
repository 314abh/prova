#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>

int addition(int x, int y);
int product(int x, int y);

/* returns the average of n float values. */
float average(size_t n, ...);

/* modifies str to convert it to an uppercase string and returns
   it. */
char *to_uppercase(char *str);
char *to_lowercase(char *str);

#endif /* CALCULATOR_H */
