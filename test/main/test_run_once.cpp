#include <unity.h>

#include "run_once.h"

// -----------------------------------------------------------------------------

TEST_CASE("RunOnce", "Expected behavior")
{
    RunOnce_t once;
    int counter = 0;

    runOnceInit(&once);

    auto cb = [](void *arg) {
        int *counter = (int *)arg;
        (*counter)++;
    };

    runOnce(&once, cb, &counter);
    runOnce(&once, cb, &counter);
    runOnce(&once, cb, &counter);

    TEST_ASSERT_EQUAL(1, counter);
}
