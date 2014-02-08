
#include "unit.h"

CONTEXT(context1,
    int i;
    int b;
){
    context1->i = 5;
    context1->b = 1;
}

CONTEXT(context2,
    int a;
){
    context2->a = 3;
}

TEST(test_1, context2)
{
    test_assert(context2->a == 3);
}

TEST(test_2, context1)
{
    test_assert(context1->b == 4, "context->b should be 3");
}

TEST(test_3, context1, context2)
{
    test_assert(context2->a == 5);

    if (!context1->b) {
        test_failed();
    } else {
        test_successful();
    }
}

TEST(test_4, context1)
{
    test_failed();
}

TEST(test_5, context1)
{
    test_failed("done bad");
}

TEST(test_6, context1)
{
    test_assert(*((int *) 1) == 5);
    test_failed("done bad");
}

RUN_ALL_TESTS();
