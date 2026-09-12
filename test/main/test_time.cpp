#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "time.h"

// -----------------------------------------------------------------------------

TEST_CASE("now_ms advances with the ESP timer", "time")
{
    uint64_t before = now_ms();

    vTaskDelay(pdMS_TO_TICKS(2));

    TEST_ASSERT_TRUE(now_ms() >= before);
}
