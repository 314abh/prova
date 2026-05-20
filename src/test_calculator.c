#include "prova.h"
#include "calculator.h"

PTEST(addition)
{
    PROVA_ASSERT_EQUAL(0, addition(0, 0));
    PROVA_ASSERT_EQUAL(0, addition(1, -1));
    PROVA_ASSERT_EQUAL(10, addition(0, 10));
    PROVA_ASSERT_EQUAL(25, addition(12, 13));
}

PTEST(product)
{
    PROVA_ASSERT_EQUAL(0, product(0, 0));
    PROVA_ASSERT_EQUAL(1, product(-1, -1));
    PROVA_ASSERT_EQUAL(200, product(20, 10));
    PROVA_ASSERT_EQUAL(-200, product(-20, 10));
}

PTEST(average)
{
    PROVA_ASSERT_EQUAL_FLOAT(2.0f, average(3, 1.0f, 2.0f, 3.0f));
    int *ptr = NULL;
    int value = *ptr;
    PROVA_ASSERT_FALSE(2 == 2);
}

PTEST(to_lowercase)
{
    char sample[] = "lower";
    PROVA_ASSERT_EQUAL_STRING("lower", to_lowercase(sample));
}

PTEST(to_uppercase)
{
    char sample[] = "upper";
    PROVA_ASSERT_EQUAL_STRING("UPPER", to_uppercase(sample));
}
