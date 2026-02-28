#include <unity.h>

#include "rundown_protection.h"

// -----------------------------------------------------------------------------

TEST_CASE("RundownProtection basic lifecycle", "acquire/release/wait transitions")
{
    RundownProtection_t rp = RUNDOWN_PROTECTION_INIT_STATIC;

    rundownProtInit(&rp);

    TEST_ASSERT_TRUE(rundownProtAcquire(&rp));
    rundownProtRelease(&rp);

    rundownProtWait(&rp);
    TEST_ASSERT_FALSE(rundownProtAcquire(&rp));

    rundownProtDestroy(&rp);
    TEST_ASSERT_NULL(rp.eg);
}

TEST_CASE("RundownProtection wait without references", "wait still marks rundown active")
{
    RundownProtection_t rp = RUNDOWN_PROTECTION_INIT_STATIC;

    rundownProtInit(&rp);
    rundownProtWait(&rp);

    TEST_ASSERT_FALSE(rundownProtAcquire(&rp));

    rundownProtDestroy(&rp);
}
