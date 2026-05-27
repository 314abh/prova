/* Copyright 2026 Abhigyan Kumar <314abh at gmail dot com>
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

#include "prova_defs.h"
#include "prova_assertions.h"

#include <threads.h>

/* Macros to register functions as test cases and add them to
 * PROVA_TEST_QUEUE. There are different shortcuts to call the main
 * PTEST_REGISTER for convinience. The PTEST_REGISTER macro takes
 * three arguments:
 *
 *   - f_name is the name of the function being tested
 *   - message is an optional comment on the test case
 *   - test_flag can be used to bypass the default flagfor new test
 *     cases. By default, test cases are assumed to be pending and are
 *     assigned the TEST_PENDING flag.
 */

#define PTEST(f_name) PTEST_REGISTER(f_name, NULL, TEST_PENDING)
#define PTEST_SKIP(f_name) PTEST_REGISTER(f_name, NULL, TEST_SKIP)
#define PTEST_MESSAGE(f_name, message)                                         \
  PTEST_REGISTER(f_name, message, TEST_PENDING)
#define PTEST_MESSAGE_SKIP(f_name, message)                                    \
  PTEST_REGISTER(f_name, message, TEST_SKIP)

#define PTEST_REGISTER(f_name, message, test_flag)                             \
  void test_method_##f_name(void);                                             \
  __attribute__((constructor)) void register_##f_name(void) {                  \
    static ProvaTest t;                                                        \
    t.status = test_flag;                                                      \
    t.test_name = #f_name;                                                     \
    t.test_message = message;                                                  \
    t.test_method = test_method_##f_name;                                      \
    stbds_arrput(PROVA_TEST_QUEUE, t);					\
  }                                                                            \
  void test_method_##f_name(void)

#endif /* PROVA_H */
