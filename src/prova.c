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

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <unistd.h>

#include "prova.h"
#include "prova_defs.h"

PTest *p_registry;
_Thread_local PTest *p_current_test;

static mtx_t print_mutex;

/* ── threaded child-process runner ── */

typedef struct
{
    PTest *test;
} PTestThreadArg;

static int prova_test_thread(void *arg)
{
    PTest *test = ((PTestThreadArg *)arg)->test;

    int pipefd[2];
    assert((pipe(pipefd) != -1) && "pipe: couldn't create pipe.");

    pid_t pid = fork();
    assert((pid >= 0) && "fork: couldn't create child process.");

    if (pid == 0)
    {
        /* ── child process (runs the actual test) ── */
        close(pipefd[0]);

        test->status = TEST_PASS;
        test->assertions_head = NULL;
        test->assertion_count = 0;
        test->fail_msg[0] = '\0';

        p_current_test = test;
        test->function();

        /* serialise: status, fail_msg, assertion_count, then each assertion (chronological) */
        write(pipefd[1], &test->status, sizeof(test->status));
        write(pipefd[1], test->fail_msg, PROVA_FAIL_MSG_MAX);

        uint32_t count = (uint32_t)test->assertion_count;
        write(pipefd[1], &count, sizeof(count));

        /* reverse the prepended list so assertions come out in execution order */
        PAssertion *prev = NULL;
        PAssertion *a = test->assertions_head;
        while (a)
        {
            PAssertion *next = a->next;
            a->next = prev;
            prev = a;
            a = next;
        }
        for (a = prev; a; a = a->next)
            write(pipefd[1], a, sizeof(PAssertion));

        close(pipefd[1]);
        _exit(0);
    }

    /* ── thread (collects results, prints immediately) ── */
    close(pipefd[1]);

    test->assertions_head = NULL;
    test->assertion_count = 0;

    PStatus child_status;
    ssize_t n = read(pipefd[0], &child_status, sizeof(child_status));

    char child_fail_msg[PROVA_FAIL_MSG_MAX];
    uint32_t child_assertion_count = 0;

    if (n == sizeof(child_status))
    {
        read(pipefd[0], child_fail_msg, PROVA_FAIL_MSG_MAX);
        read(pipefd[0], &child_assertion_count, sizeof(child_assertion_count));
    }

    /* rebuild a forward-linked list from the serialised assertions */
    if (n == sizeof(child_status))
    {
        PAssertion **tail = &test->assertions_head;
        for (uint32_t i = 0; i < child_assertion_count; i++)
        {
            PAssertion *a = (PAssertion *)malloc(sizeof(PAssertion));
            read(pipefd[0], a, sizeof(PAssertion));
            a->next = NULL;
            *tail = a;
            tail = &a->next;
        }
    }
    test->assertion_count = child_assertion_count;

    close(pipefd[0]);

    int wstatus;
    waitpid(pid, &wstatus, 0);

    if (WIFSIGNALED(wstatus))
    {
        test->status = TEST_CRASH;
        snprintf(test->fail_msg, PROVA_FAIL_MSG_MAX, "signal %d (%s)",
                 WTERMSIG(wstatus), strsignal(WTERMSIG(wstatus)));
    }
    else if (n != sizeof(child_status))
    {
        test->status = TEST_CRASH;
        snprintf(test->fail_msg, PROVA_FAIL_MSG_MAX,
                 "child exited without writing test results");
    }
    else
    {
        test->status = child_status;
        memcpy(test->fail_msg, child_fail_msg, PROVA_FAIL_MSG_MAX);
    }

    /* print under mutex so output from concurrent threads doesn't interleave */
    mtx_lock(&print_mutex);
    {
        switch (test->status)
        {
        case TEST_PASS:
            printf(PROVA_PASSED_TAG " %s", test->name);
            if (test->msg)
                printf(" : %s", test->msg);
            printf("\n");
            break;
        case TEST_FAIL:
            printf(PROVA_FAILED_TAG " %s : %s\n", test->name, test->fail_msg);
            break;
        case TEST_CRASH:
            printf(PROVA_CRASHED_TAG " %s : %s\n", test->name, test->fail_msg);
            break;
        default:
            break;
        }

        for (PAssertion *a = test->assertions_head; a; a = a->next)
        {
            if (a->status == TEST_PASS)
                printf("    " PROVA_ANSI_GREEN "\xe2\x9c\x93" PROVA_ANSI_RESET " %s\n",
                       a->expression);
            else
                printf("    " PROVA_ANSI_RED "\xe2\x9c\x97" PROVA_ANSI_RESET
                       " %s  " PROVA_ANSI_RED "(line %zu)" PROVA_ANSI_RESET "\n",
                       a->expression, a->line);
        }
        fflush(stdout);
    }
    mtx_unlock(&print_mutex);

    return 0;
}

/* ── public API ── */

void prova_run_tests(PTest *registry)
{
    size_t total = prova_count_tests(registry);
    if (total == 0)
        return;

    thrd_t *threads = (thrd_t *)malloc(total * sizeof(thrd_t));
    PTestThreadArg *args = (PTestThreadArg *)malloc(total * sizeof(PTestThreadArg));

    mtx_init(&print_mutex, mtx_plain);

    /* print skipped tests immediately (no fork needed) */
    for (PTest *cur = registry; cur; cur = cur->next)
    {
        if (cur->status == TEST_SKIP)
            printf(PROVA_SKIPPED_TAG " %s\n", cur->name);
    }

    /* launch one thread per non-skipped test */
    size_t nthreads = 0;
    size_t idx = 0;
    for (PTest *cur = registry; cur; cur = cur->next)
    {
        if (cur->status == TEST_SKIP)
            continue;
        args[idx].test = cur;
        if (thrd_create(&threads[idx], prova_test_thread, &args[idx]) == thrd_success)
        {
            idx++;
            nthreads++;
        }
    }

    /* wait for all test threads to finish */
    for (size_t i = 0; i < nthreads; i++)
        thrd_join(threads[i], NULL);

    mtx_destroy(&print_mutex);

    /* final summary */
    unsigned int count_total = 0, passed = 0, failed = 0, crashed = 0, skipped = 0;
    for (PTest *cur = registry; cur; cur = cur->next)
    {
        count_total++;
        switch (cur->status)
        {
        case TEST_PASS:
            passed++;
            break;
        case TEST_FAIL:
            failed++;
            break;
        case TEST_CRASH:
            crashed++;
            break;
        case TEST_SKIP:
            skipped++;
            break;
        default:
            break;
        }
    }

    unsigned int run_total = passed + failed + crashed;
    printf(PROVA_ANSI_BOLD "\nTest Summary" PROVA_ANSI_RESET "\n");
    printf("  Total:   %u\n", count_total);
    printf("  Passed:  " PROVA_ANSI_GREEN "%u" PROVA_ANSI_RESET "\n", passed);
    printf("  Failed:  " PROVA_ANSI_RED "%u" PROVA_ANSI_RESET "\n", failed);
    printf("  Crashed: " PROVA_ANSI_MAGENTA "%u" PROVA_ANSI_RESET "\n", crashed);
    printf("  Skipped: " PROVA_ANSI_CYAN "%u" PROVA_ANSI_RESET "\n", skipped);
    printf("  Rate:    %u/%u\n", passed, run_total);

    free(threads);
    free(args);
}

size_t prova_count_tests(const PTest *registry)
{
    size_t n = 0;
    for (const PTest *cur = registry; cur; cur = cur->next)
        n++;
    return n;
}

void prova_print_summary(PTest *registry)
{
    unsigned int total = 0, passed = 0, failed = 0, crashed = 0, skipped = 0;
    for (const PTest *cur = registry; cur; cur = cur->next)
    {
        total++;
        switch (cur->status)
        {
        case TEST_PASS:
            passed++;
            break;
        case TEST_FAIL:
            failed++;
            break;
        case TEST_CRASH:
            crashed++;
            break;
        case TEST_SKIP:
            skipped++;
            break;
        default:
            break;
        }
    }
    unsigned int run_total = passed + failed + crashed;
    printf(PROVA_ANSI_BOLD "\nTest Summary" PROVA_ANSI_RESET "\n");
    printf("  Total:   %u\n", total);
    printf("  Passed:  " PROVA_ANSI_GREEN "%u" PROVA_ANSI_RESET "\n", passed);
    printf("  Failed:  " PROVA_ANSI_RED "%u" PROVA_ANSI_RESET "\n", failed);
    printf("  Crashed: " PROVA_ANSI_MAGENTA "%u" PROVA_ANSI_RESET "\n", crashed);
    printf("  Skipped: " PROVA_ANSI_CYAN "%u" PROVA_ANSI_RESET "\n", skipped);
    printf("  Rate:    %u/%u\n", passed, run_total);
}

void prova_cleanup(PTest *registry)
{
    for (PTest *cur = registry; cur; cur = cur->next)
    {
        PAssertion *a = cur->assertions_head;
        while (a)
        {
            PAssertion *next = a->next;
            free(a);
            a = next;
        }
        cur->assertions_head = NULL;
        cur->assertion_count = 0;
    }
}

#ifndef PROVA_MAIN
#define PROVA_MAIN

int main(void)
{
    prova_run_tests(p_registry);
    unsigned int fail_count = 0;
    for (PTest *cur = p_registry; cur; cur = cur->next)
    {
        if (cur->status == TEST_FAIL || cur->status == TEST_CRASH)
            fail_count++;
    }
    prova_cleanup(p_registry);
    return fail_count > 0 ? 1 : 0;
}

#endif /* PROVA_MAIN */
