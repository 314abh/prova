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

#ifndef PROVA_DEFS_H
#define PROVA_DEFS_H

#define PROVA_FAIL_MSG_MAX 512
#define PROVA_ASSERT_ARRAY_BUFFER 1024

#ifndef PROVA_FLOAT_EPSILON
#define PROVA_FLOAT_EPSILON 10e-5
#endif /* PROVA_FLOAT_EPSILON */

/* === ANSI COLOR SEQUENCES START === */

#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_MAGENTA "\x1b[35m"

#ifdef PROVA_NO_COLORS

#define PROVA_PASSED_TAG  "[PASSED] "
#define PROVA_FAILED_TAG  "[FAILED] "
#define PROVA_CRASHED_TAG "[CRASHED]"
#define PROVA_PENDING_TAG "[PENDING]"
#define PROVA_SKIPPED_TAG "[SKIPPED]"

#else

#define PROVA_PASSED_TAG  ANSI_BOLD ANSI_GREEN   "[PASSED] "  ANSI_RESET
#define PROVA_FAILED_TAG  ANSI_BOLD ANSI_RED     "[FAILED] "  ANSI_RESET
#define PROVA_CRASHED_TAG ANSI_BOLD ANSI_MAGENTA "[CRASHED]" ANSI_RESET
#define PROVA_PENDING_TAG ANSI_BOLD ANSI_YELLOW  "[PENDING]" ANSI_RESET
#define PROVA_SKIPPED_TAG ANSI_BOLD ANSI_CYAN    "[SKIPPED]" ANSI_RESET

#endif /* PROVA_NO_COLORS */

/* ===  ANSI COLOR SEQUENCES END  === */

#endif /* PROVA_DEFS_H */
