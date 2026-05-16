Unit testing framework in C, for C. The focus is on simplicity and
flexibility, enough that on can edit the source files of the unit
testing framework itself and bend it according to their needs.

Currently works and tested with only `clang` and `gcc`.

### Features

- [X] basic assertion tests
- [X] test execution in isolated child process
- [ ] multithreaded
- [ ] cross-platform
- [ ] works for C++ projects too

### Unit Assertion Macros

Names of all unit assertion macros are self-explanatory. Each macro's name reveals for what expression it will *not* fail. For example, `PROVA_ASSERT_EQUAL_PTR(expected, actual)` macro implies that the test won't fail if the pointer values `expected` and `actual` are equal in value.

**Primitive assertions**

1. `PROVA_ASSERT_TRUE(expr)` fails if `expr` is false
2. `PROVA_ASSERT_FALSE(expr)` fails if `expr` is true
3. `PROVA_ASSERT_NULL(ptr)` fails if `ptr` is not NULL
4. `PROVA_ASSERT_NOT_NULL(ptr)` fails if `ptr` is NULL

**Generic assertions**

Use with signed and unsigned integer types or data types comparable with common comparison operators in C like `<`, `<=` and `>`, `>=`. Pointers are cast to `void*` before comparison

1. `PROVA_ASSERT_EQUAL_PTR(expected, actual)` fails if pointer values `expected` and `actual` are not equal
2. `PROVA_ASSERT_NOT_EQUAL_PTR(expected, actual)` fails if pointer values `expected` and `actual` are equal
3. `PROVA_ASSERT_EQUAL(expected, actual)` fails if the values `expected` and `actual` are not equal
4. `PROVA_ASSERT_NOT_EQUAL(expected, actual)` fails if the values `expected` and `actual` are equal
5. `PROVA_ASSERT_GREATER_THAN(threshold, actual)` fails if the value `actual` is not greater than the `threshold` value
6. `PROVA_ASSERT_GREATER_EQUAL_THAN(threshould, actual)` fails if the value `actual` is smaller than the `threshold` value
7. `PROVA_ASSERT_LESS_THAN(threshold, actual)` fails if the value `threshold` is not greater than the `actual` value
8. `PROVA_ASSERT_LESS_EQUAL_THAN(threshold, actual)` fails if the value `actual` is greater than the `threshold` value

**Float assertions**

The default value of `PROVA_FLOAT_EPSILON` is `10e-5`.

1. `PROVA_ASSERT_EQUAL_FLOAT(expected, actual)` compares the floating values `expected` and `actual` within the error margin of `PROVA_FLOAT_EPSILON`
2. `PROVA_ASSERT_NOT_EQUAL_FLOAT_WITHIN(delta, expected, actual)` compares the floating values `expected` and within an error margin of `delta`

**Memory assertions**

1. `PROVA_ASSERT_EQUAL_STRING(expected, actual)` fails if the string values pointed to by `expected` and `actual` are not equal in bytes
2. `PROVA_ASSERT_EQUAL_STRING_LENGTH(expected, actual)` fails if the string values pointed to by `expected` and `actual` are not equal in length
3. `PROVA_ASSERT_NOT_EQUAL_STRING(expected, actual)` fails if the string values pointed to by `expected` and `actual` are equal in bytes
4. `PROVA_ASSERT_NOT_EQUAL_STRING_LENGTH(expected, actual)` fails if the string values pointed to by `expected` and `actual` are equal in length
5. `PROVA_ASSERT_EQUAL_ARRAYS(expected, actual, n)` fails if the contents of the arrays `expected` and `actual` are not equal. `n` denotes the least of the sizes of both the arrays. Fails at the first mismatch
6. `PROVA_ASSERT_NOT_EQUAL_ARRAYS(expected, actual, n)` fails if the contents of the arrays `expected` and `actual` are equal. `n` denotes the least of the sizes of both the arrays. Fails at the first mismatch
7. `PROVA_ASSERT_EQUAL_MEMORY(expected, actual, n)` fails if the contents of the memory blocks `expected` and `actual` are not equal. `n` denotes the least of the sizes of both the arrays
8. `PROVA_ASSERT_NOT_EQUAL_MEMORY(expected, actual, n)` fails if the contents of the memory blocks `expected` and `actual` are equal. `n` denotes the least of the sizes of both the arrays


### Copyright

Apache License v2.0. Copyright (c) 2026 Abhigyan Kumar \<314abh at gmail dot com\>.
