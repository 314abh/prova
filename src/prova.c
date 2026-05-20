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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <unistd.h>

#include "prova.h"
#include "prova_defs.h"

/* global structs */
PMeta p_metadata;
PTest *p_registry;
thread_local PAssertCtx *p_assert_ctx;

static const char *prova_test_summary(PTest *test, char buffer[], size_t buffer_size);

/* TODO: implement cleaner method to report errors related to pipes and forks.
 */
void prova_run_tests(PTest *registry)
{
    PTest *curr = registry;
    size_t total_tests = prova_count_tests(curr);
    p_metadata.total_tests = total_tests;

    while (curr)
    {
        if (curr->status == TEST_SKIP)
        {
            p_metadata.skipping_tests++;
            char print_msg_buffer[5000 + sizeof(curr->msg)];
            prova_test_summary(curr, print_msg_buffer, sizeof(print_msg_buffer));
            fputs(print_msg_buffer, stdout);
            curr = curr->next;
            continue;
        }

        int pipefd[2];
        assert((pipe(pipefd) != -1) && "pipe: couldn't create pipe for new child process.");

        pid_t pid = fork();
        assert((pid >= 0) && "fork: couldn't create child process.");

        if (pid == 0)
        {
            close(pipefd[0]);

            /* execute test function within local context. assume passing by default. */
            PAssertCtx local_ctx;
            memset(&local_ctx, 0, sizeof(local_ctx));
            p_assert_ctx = &local_ctx;
            curr->function();

            write(pipefd[1], p_assert_ctx, sizeof(*p_assert_ctx));
            close(pipefd[1]);
            _exit(local_ctx.status == TEST_PASS ? 0 : 1);
        }
        else
        {
            close(pipefd[1]);
            PAssertCtx local_ctx;
            memset(&local_ctx, 0, sizeof(local_ctx));
            ssize_t n = read(pipefd[0], &local_ctx, sizeof(local_ctx));
            close(pipefd[0]);

            int wstatus;
            waitpid(pid, &wstatus, 0);
            if (n == sizeof(local_ctx) && local_ctx.status == TEST_PASS)
            {
                curr->status = TEST_PASS;
                p_metadata.passing_tests++;
            }
            else if (WIFSIGNALED(wstatus))
            {
                curr->status = TEST_CRASH;
                p_metadata.crashing_tests++;
            }
            else
            {
                curr->status = TEST_FAIL;
                p_metadata.failing_tests++;
                char *s = (char *)malloc(PROVA_FAIL_MSG_MAX);
                strncpy(s, local_ctx.fail_msg, PROVA_FAIL_MSG_MAX);
                curr->msg = s;
            }

            char print_msg_buffer[5000 + sizeof(curr->msg)];
            prova_test_summary(curr, print_msg_buffer, sizeof(print_msg_buffer));
            fputs(print_msg_buffer, stdout);
        }

        curr = curr->next;
    }
}

size_t prova_count_tests(const PTest *registry)
{
    const PTest *curr = registry;
    size_t total = 0;
    while (curr)
    {
        total++;
        curr = curr->next;
    }

    return total;
}

void prova_print_summary()
{
    printf("\n=== SUMMARY ===\n");
    printf("Total testcases: %u\n", p_metadata.total_tests);
    printf("Failing testcases: %u\n", p_metadata.failing_tests);
    printf("Skipped testcases: %u\n", p_metadata.skipping_tests);
    printf("Crashing testcases: %u\n", p_metadata.crashing_tests);
    printf("Passing ratio: %u/%u\n", p_metadata.passing_tests, p_metadata.total_tests);
}

void prova_cleanup_messages(const PTest *registry)
{
    const PTest *curr = registry;
    while (curr)
    {
        PTest *next = curr->next;
        free(curr->msg);
        curr = next;
    }
}

static const char *prova_test_summary(PTest *test, char buffer[], size_t buffer_size)
{
    size_t print_size;
    const char ellipsis[] = "...";
    switch (test->status)
    {
    case TEST_FAIL:
        print_size = snprintf(buffer, buffer_size, PROVA_FAILED_TAG " unit %s : %s\n", test->name, test->msg);
        break;
    case TEST_CRASH:
        print_size = snprintf(buffer, buffer_size, PROVA_CRASHED_TAG " unit %s : %s\n", test->name, test->msg);
        break;
    case TEST_SKIP:
        print_size = snprintf(buffer, buffer_size, PROVA_SKIPPED_TAG " unit %s : %s\n", test->name, test->msg);
        break;
    case TEST_PASS:
        print_size = snprintf(buffer, buffer_size, PROVA_PASSED_TAG " unit %s : %s\n", test->name, test->msg);
        break;
    case TEST_PENDING:
        print_size = snprintf(buffer, buffer_size, PROVA_PENDING_TAG " unit %s : %s\n", test->name, test->msg);
        break;
    default:
        assert(false && "couldn't validate test status.");
    }

    if (print_size >= buffer_size)
    {
        /* + 2 for null-terminator and newline. */
        strncpy(buffer + (buffer_size - (sizeof(ellipsis) + 2)), ellipsis, sizeof(ellipsis));
    }

    return buffer;
}

#ifndef PROVA_MAIN
#define PROVA_MAIN

int main(void)
{
    prova_run_tests(p_registry);
    prova_print_summary();
    return (p_metadata.failing_tests + p_metadata.crashing_tests) > 0 ? 1 : 0;
}

#endif /* PROVA_MAIN */
