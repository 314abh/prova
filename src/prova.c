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

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "prova.h"
#include "prova_defs.h"

/* global registry (built at static-init time, read-only after main) */
PTest *p_registry;

/* per-thread pointer to the currently executing PTest */
_Thread_local PTest *p_current_test;

/* per-thread crash recovery jump buffer */
static _Thread_local sigjmp_buf prova_crash_jmp;

/* per-thread flag: did we already longjmp? prevents re-entrant handlers */
static _Thread_local int prova_crashed;

static void prova_crash_handler(int sig)
{
    if (p_current_test && !prova_crashed)
    {
        prova_crashed = 1;
        p_current_test->status = TEST_CRASH;
        if (sig == SIGSEGV)
            snprintf(p_current_test->fail_msg, PROVA_FAIL_MSG_MAX,
                     "SIGSEGV (invalid memory access)");
        else if (sig == SIGFPE)
            snprintf(p_current_test->fail_msg, PROVA_FAIL_MSG_MAX,
                     "SIGFPE (floating-point exception)");
        else if (sig == SIGILL)
            snprintf(p_current_test->fail_msg, PROVA_FAIL_MSG_MAX,
                     "SIGILL (illegal instruction)");
        else if (sig == SIGABRT)
            snprintf(p_current_test->fail_msg, PROVA_FAIL_MSG_MAX,
                     "SIGABRT (aborted)");
        else if (sig == SIGBUS)
            snprintf(p_current_test->fail_msg, PROVA_FAIL_MSG_MAX,
                     "SIGBUS (bus error)");
        else
            snprintf(p_current_test->fail_msg, PROVA_FAIL_MSG_MAX,
                     "signal %d", sig);
    }

    /* restore default and re-raise so the whole process dies if we somehow
     * return from longjmp handler. The longjmp below should normally prevent
     * reaching here. */
    signal(sig, SIG_DFL);

    siglongjmp(prova_crash_jmp, 1);
}

static void prova_install_crash_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = prova_crash_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
}

static int prova_run_single_test(void *arg)
{
    PTest *test = (PTest *)arg;
    p_current_test = test;
    prova_crashed = 0;

    prova_install_crash_handlers();

    /* reset runtime state before running */
    test->status = TEST_PASS;
    test->assertions_head = NULL;
    test->assertion_count = 0;
    test->fail_msg[0] = '\0';

    if (sigsetjmp(prova_crash_jmp, 1) == 0)
    {
        test->function();
    }

    return 0;
}

void prova_run_tests(PTest *registry)
{
    size_t total = prova_count_tests(registry);
    if (total == 0)
        return;

    thrd_t *threads = (thrd_t *)malloc(total * sizeof(thrd_t));
    PTest **order = (PTest **)malloc(total * sizeof(PTest *));

    /* flatten linked list into ordered array */
    size_t idx = 0;
    for (PTest *cur = registry; cur; cur = cur->next)
        order[idx++] = cur;

    /* launch one thread per non-skipped test */
    for (size_t i = 0; i < total; i++)
    {
        if (order[i]->status == TEST_SKIP)
        {
            threads[i] = (thrd_t)0;
            continue;
        }
        thrd_create(&threads[i], prova_run_single_test, order[i]);
    }

    /* collect results and print per-test summary */
    unsigned int passed = 0, failed = 0, crashed = 0, skipped = 0;

    for (size_t i = 0; i < total; i++)
    {
        PTest *t = order[i];

        if (t->status == TEST_SKIP)
        {
            /* still need to print skipped tests */
            printf(PROVA_SKIPPED_TAG " %s\n", t->name);
            skipped++;
            continue;
        }

        int result;
        thrd_join(threads[i], &result);

        /* If the test didn't explicitly fail but crashed was set by handler */
        if (t->status == TEST_CRASH)
        {
            printf(PROVA_CRASHED_TAG " %s : %s\n", t->name, t->fail_msg);
            crashed++;
        }
        else if (t->status == TEST_FAIL)
        {
            printf(PROVA_FAILED_TAG " %s : %s\n", t->name, t->fail_msg);
            failed++;
        }
        else
        {
            printf(PROVA_PASSED_TAG " %s", t->name);
            if (t->msg)
                printf(" : %s", t->msg);
            printf("\n");
            passed++;
        }

        /* print each assertion for this test */
        if (t->assertions_head)
        {
            /* reverse the list (we prepended during recording) */
            PAssertion *prev = NULL;
            PAssertion *cur = t->assertions_head;
            while (cur)
            {
                PAssertion *next = cur->next;
                cur->next = prev;
                prev = cur;
                cur = next;
            }
            t->assertions_head = prev;

            for (PAssertion *a = t->assertions_head; a; a = a->next)
            {
                if (a->status == TEST_PASS)
                    printf("    " PROVA_ANSI_GREEN "\xe2\x9c\x93" PROVA_ANSI_RESET
                           " %s\n",
                           a->expression);
                else
                    printf("    " PROVA_ANSI_RED "\xe2\x9c\x97" PROVA_ANSI_RESET
                           " %s  " PROVA_ANSI_RED "(line %zu)" PROVA_ANSI_RESET "\n",
                           a->expression, a->line);
            }
        }
    }

    free(threads);
    free(order);

    /* print final summary */
    unsigned int total_run = passed + failed + crashed;
    printf(PROVA_ANSI_BOLD "\nTest Summary" PROVA_ANSI_RESET "\n");
    printf("  Total:  %zu\n", total);
    printf("  Passed: " PROVA_ANSI_GREEN "%u" PROVA_ANSI_RESET "\n", passed);
    printf("  Failed: " PROVA_ANSI_RED "%u" PROVA_ANSI_RESET "\n", failed);
    printf("  Crashed: " PROVA_ANSI_MAGENTA "%u" PROVA_ANSI_RESET "\n", crashed);
    printf("  Skipped: " PROVA_ANSI_CYAN "%u" PROVA_ANSI_RESET "\n", skipped);
    printf("  Rate:   %u/%u\n", passed, total_run);
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
    unsigned int total_run = passed + failed + crashed;
    printf(PROVA_ANSI_BOLD "\nTest Summary" PROVA_ANSI_RESET "\n");
    printf("  Total:   %u\n", total);
    printf("  Passed:  " PROVA_ANSI_GREEN "%u" PROVA_ANSI_RESET "\n", passed);
    printf("  Failed:  " PROVA_ANSI_RED "%u" PROVA_ANSI_RESET "\n", failed);
    printf("  Crashed: " PROVA_ANSI_MAGENTA "%u" PROVA_ANSI_RESET "\n", crashed);
    printf("  Skipped: " PROVA_ANSI_CYAN "%u" PROVA_ANSI_RESET "\n", skipped);
    printf("  Rate:    %u/%u\n", passed, total_run);
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
