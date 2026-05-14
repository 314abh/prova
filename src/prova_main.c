/* prova_main.c */
#include "prova.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    prova_run_tests(p_registry);
    prova_print_summary(p_registry);
    return (p_metadata.failing_tests + p_metadata.crashing_tests) > 0 ? 1 : 0;
}
