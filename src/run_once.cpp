#include "run_once.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define RUN_ONCE_NOT_STARTED 0
#define RUN_ONCE_RUNNING     1
#define RUN_ONCE_FINISHED    2

// -----------------------------------------------------------------------------

void runOnceInit(RunOnce_t *once)
{
    once->store(0);
}

void runOnce(RunOnce_t *once, RunOnceCallback_t cb, const void *arg)
{
    uint32_t expected = RUN_ONCE_NOT_STARTED;
    if (once->compare_exchange_strong(expected, RUN_ONCE_RUNNING)) {
        cb(arg);
        once->store(RUN_ONCE_FINISHED);
    }
    else if (expected == RUN_ONCE_RUNNING) {
        // If running, spin until done
        while (once->load() != RUN_ONCE_FINISHED) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}
