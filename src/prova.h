/* Copyright 2026 Abhigyan Kumar Abhigyan Kumar <314abh at gmail dot com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PROVA_H
#define PROVA_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#include "prova_defs.h"

typedef enum PStatus
{
    /* TEST_PASS should be zero valued, otherwise memset on local_ctx fails in prova_run_tests */
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP,
    TEST_CRASH,
    TEST_PENDING
} PStatus;

/* A single assertion recorded during a test run. */
typedef struct PAssertion
{
    size_t line;
    PStatus status;
    struct PAssertion *next;
    char expression[PROVA_EXPR_LEN_MAX];
} PAssertion;

/* Metadata for a unit test.
 * status: initial (PENDING/SKIP) set at registration; updated at runtime.
 * name: name of the function to be tested.
 * msg: optional user-provided description (string literal, set at registration).
 * function: pointer to the test function itself.
 * next: next item in the linked list.
 * assertions_head: linked list of every assertion evaluated during the run.
 * assertion_count: total number of assertions evaluated.
 * fail_msg: populated at runtime with the first failure location if any.
 */
typedef struct PTest
{
    PStatus status;
    const char *name;
    const char *msg;
    void (*function)(void);
    struct PTest *next;
    PAssertion *assertions_head;
    size_t assertion_count;
    char fail_msg[PROVA_FAIL_MSG_MAX];
} PTest;

extern PTest *p_registry;
extern _Thread_local PTest *p_current_test;

void prova_print_summary(PTest *registry);
void prova_run_tests(PTest *registry);
size_t prova_count_tests(const PTest *registry);
void prova_cleanup(PTest *registry);

/* Record a single assertion result (PASS or FAIL) into the current test. */
static inline void prova_record_assertion(uint32_t line, const char *file, const char *expr, bool passed)
{
    if (p_current_test == NULL)
        return;

    PAssertion *a = (PAssertion *)malloc(sizeof(PAssertion));
    memset(a, 0, sizeof(PAssertion));
    a->line = line;
    a->status = passed ? TEST_PASS : TEST_FAIL;
    a->next = p_current_test->assertions_head;
    size_t len = strlen(expr);
    if (len >= PROVA_EXPR_LEN_MAX)
        len = PROVA_EXPR_LEN_MAX - 1;
    memcpy(a->expression, expr, len);
    p_current_test->assertions_head = a;
    p_current_test->assertion_count++;

    if (!passed)
    {
        p_current_test->status = TEST_FAIL;
        snprintf(p_current_test->fail_msg, PROVA_FAIL_MSG_MAX, "%s:%d: %s", file, line, expr);
    }
}

#define PTEST(f_name) PTEST_REGISTER(f_name, NULL, TEST_PENDING)
#define PTEST_SKIP(f_name) PTEST_REGISTER(f_name, NULL, TEST_SKIP)
#define PTEST_MESSAGE(f_name, message) PTEST_REGISTER(f_name, message, TEST_PENDING)
#define PTEST_MESSAGE_SKIP(f_name, message) PTEST_REGISTER(f_name, message, TEST_SKIP)

#define PTEST_REGISTER(f_name, message, test_flag)                                                                     \
    void test_function_##f_name(void);                                                                                 \
    __attribute__((constructor)) void register_##f_name(void)                                                          \
    {                                                                                                                  \
        static PTest t;                                                                                                \
        t.status = test_flag;                                                                                          \
        t.name = #f_name;                                                                                              \
        t.msg = message;                                                                                               \
        t.function = test_function_##f_name;                                                                           \
        t.next = p_registry;                                                                                           \
        t.assertions_head = NULL;                                                                                      \
        t.assertion_count = 0;                                                                                         \
        t.fail_msg[0] = '\0';                                                                                          \
        p_registry = &t;                                                                                               \
    }                                                                                                                  \
    void test_function_##f_name(void)

/* === COMMON ASSERTION MACROS START === */

#define PROVA_ASSERT(expr)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = !!(expr);                                                                                     \
        prova_record_assertion(__LINE__, __FILE__, #expr, _prova_ok);                                                   \
    } while (0)

#define PROVA_ASSERT_TRUE(expr) PROVA_ASSERT((expr) != false)
#define PROVA_ASSERT_FALSE(expr) PROVA_ASSERT((expr) == false)
#define PROVA_ASSERT_NULL(ptr) PROVA_ASSERT((ptr) == NULL)
#define PROVA_ASSERT_NOT_NULL(ptr) PROVA_ASSERT((ptr) != NULL)

#define PROVA_ASSERT_EQUAL_PTR(expected, actual)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (void *)(expected) == (void *)(actual);                                                       \
        prova_record_assertion(__LINE__, __FILE__, #actual " == " #expected, _prova_ok);                               \
    } while (0)

#define PROVA_ASSERT_NOT_EQUAL_PTR(expected, actual)                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (void *)(expected) != (void *)(actual);                                                       \
        prova_record_assertion(__LINE__, __FILE__, #actual " != " #expected, _prova_ok);                               \
    } while (0)

#define PROVA_ASSERT_EQUAL(expected, actual)                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (expected) == (actual);                                                                       \
        prova_record_assertion(__LINE__, __FILE__, #actual " == " #expected, _prova_ok);                               \
    } while (0)

#define PROVA_ASSERT_NOT_EQUAL(expected, actual)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (expected) != (actual);                                                                       \
        prova_record_assertion(__LINE__, __FILE__, #actual " != " #expected, _prova_ok);                               \
    } while (0)

#define PROVA_ASSERT_GREATER_THAN(threshold, actual)                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (actual) > (threshold);                                                                       \
        prova_record_assertion(__LINE__, __FILE__, #actual " > " #threshold, _prova_ok);                               \
    } while (0)

#define PROVA_ASSERT_GREATER_EQUAL_THAN(threshold, actual)                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (actual) >= (threshold);                                                                      \
        prova_record_assertion(__LINE__, __FILE__, #actual " >= " #threshold, _prova_ok);                              \
    } while (0)

#define PROVA_ASSERT_LESS_THAN(threshold, actual)                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (actual) < (threshold);                                                                       \
        prova_record_assertion(__LINE__, __FILE__, #actual " < " #threshold, _prova_ok);                               \
    } while (0)

#define PROVA_ASSERT_LESS_EQUAL_THAN(threshold, actual)                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = (actual) <= (threshold);                                                                      \
        prova_record_assertion(__LINE__, __FILE__, #actual " <= " #threshold, _prova_ok);                              \
    } while (0)

#define PROVA_ASSERT_EQUAL_FLOAT(expected, actual)                                                                     \
    PROVA_ASSERT_EQUAL_FLOAT_WITHIN(PROVA_FLOAT_EPSILON, (expected), (actual))

#define PROVA_ASSERT_EQUAL_FLOAT_WITHIN(delta, expected, actual)                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        float _e = (float)(expected);                                                                                  \
        float _a = (float)(actual);                                                                                    \
        float _diff = fabsf(_a - _e);                                                                                  \
        float _max = fmaxf(fabsf(_e), fabsf(_a));                                                                      \
        bool _prova_ok = (_max == 0.0f) ? (_diff <= (delta)) : (_diff <= (delta) * _max);                              \
        prova_record_assertion(__LINE__, __FILE__, #actual " ~= " #expected " (\xC2\xB1" #delta ")", _prova_ok);        \
    } while (0)

#define PROVA_ASSERT_NOT_EQUAL_FLOAT(expected, actual)                                                                 \
    PROVA_ASSERT_NOT_EQUAL_FLOAT_WITHIN(PROVA_FLOAT_EPSILON, (expected), (actual))

#define PROVA_ASSERT_NOT_EQUAL_FLOAT_WITHIN(delta, expected, actual)                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        float _e = (float)(expected);                                                                                  \
        float _a = (float)(actual);                                                                                    \
        float _diff = fabsf(_a - _e);                                                                                  \
        float _max = fmaxf(fabsf(_e), fabsf(_a));                                                                      \
        bool _prova_ok = (_max == 0.0f) ? (_diff > (delta)) : (_diff > (delta) * _max);                                \
        prova_record_assertion(__LINE__, __FILE__, #actual " !~= " #expected " (\xC2\xB1" #delta ")", _prova_ok);       \
    } while (0)

#define PROVA_ASSERT_EQUAL_STRING(expected, actual)                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        const char *_prova_e = (expected);                                                                             \
        const char *_prova_a = (actual);                                                                               \
        bool _prova_ok = (_prova_e == _prova_a) || (_prova_e && _prova_a && strcmp(_prova_e, _prova_a) == 0);          \
        prova_record_assertion(__LINE__, __FILE__, "strcmp(" #actual ", " #expected ") == 0", _prova_ok);              \
    } while (0)

#define PROVA_ASSERT_EQUAL_STRING_LENGTH(expected, actual)                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = strlen(expected) == strlen(actual);                                                           \
        prova_record_assertion(__LINE__, __FILE__,                                                                     \
                               "strlen(" #actual ") == strlen(" #expected ")", _prova_ok);                             \
    } while (0)

#define PROVA_ASSERT_NOT_EQUAL_STRING(expected, actual)                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        const char *_prova_e = (expected);                                                                             \
        const char *_prova_a = (actual);                                                                               \
        bool _prova_ok = (_prova_e != _prova_a) && (_prova_e == NULL || _prova_a == NULL || strcmp(_prova_e, _prova_a) != 0); \
        prova_record_assertion(__LINE__, __FILE__, "strcmp(" #actual ", " #expected ") != 0", _prova_ok);              \
    } while (0)

#define PROVA_ASSERT_NOT_EQUAL_STRING_LENGTH(expected, actual)                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = strlen(expected) != strlen(actual);                                                           \
        prova_record_assertion(__LINE__, __FILE__,                                                                     \
                               "strlen(" #actual ") != strlen(" #expected ")", _prova_ok);                             \
    } while (0)

#define PROVA_ASSERT_EQUAL_ARRAYS(expected, actual, n)                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        char _prova_buf[PROVA_ASSERT_ARRAY_BUFFER];                                                                    \
        for (size_t i = 0; i < n; ++i)                                                                                 \
        {                                                                                                              \
            bool _prova_ok = (expected)[i] == (actual)[i];                                                             \
            snprintf(_prova_buf, PROVA_ASSERT_ARRAY_BUFFER, #actual "[%zu] == " #expected "[%zu]", i, i);              \
            prova_record_assertion(__LINE__, __FILE__, _prova_buf, _prova_ok);                                         \
        }                                                                                                              \
    } while (0)

#define PROVA_ASSERT_NOT_EQUAL_ARRAYS(expected, actual, n)                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        char _prova_buf[PROVA_ASSERT_ARRAY_BUFFER];                                                                    \
        for (size_t i = 0; i < n; ++i)                                                                                 \
        {                                                                                                              \
            bool _prova_ok = (expected)[i] != (actual)[i];                                                             \
            snprintf(_prova_buf, PROVA_ASSERT_ARRAY_BUFFER, #actual "[%zu] != " #expected "[%zu]", i, i);              \
            prova_record_assertion(__LINE__, __FILE__, _prova_buf, _prova_ok);                                         \
        }                                                                                                              \
    } while (0)

#define PROVA_ASSERT_EQUAL_MEMORY(expected, actual, n)                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = memcmp(expected, actual, n) == 0;                                                             \
        prova_record_assertion(__LINE__, __FILE__, "memcmp(" #actual ", " #expected ", " #n ") == 0", _prova_ok);      \
    } while (0)

#define PROVA_ASSERT_NOT_EQUAL_MEMORY(expected, actual, n)                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        bool _prova_ok = memcmp(expected, actual, n) != 0;                                                             \
        prova_record_assertion(__LINE__, __FILE__, "memcmp(" #actual ", " #expected ", " #n ") != 0", _prova_ok);      \
    } while (0)

/* ===  COMMON ASSERTION MACROS END  === */

#endif /* PROVA_H */
