#include "run_once.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define RUN_ONCE_NOT_STARTED 0
#define RUN_ONCE_RUNNING     1
#define RUN_ONCE_FINISHED    2

// -----------------------------------------------------------------------------

void runOnceInit(RunOnce_t *once)
{
    atomic_store_explicit(once, RUN_ONCE_NOT_STARTED, memory_order_relaxed);
}

void runOnce(RunOnce_t *once, RunOnceCallback_t cb, void *ctx)
{
    uint32_t expected = RUN_ONCE_NOT_STARTED;

    if (atomic_compare_exchange_strong_explicit(once, &expected, RUN_ONCE_RUNNING,
                                                memory_order_acq_rel, memory_order_acquire))
    {

        cb(ctx);
        atomic_store_explicit(once, RUN_ONCE_FINISHED, memory_order_release);
    }
    else if (expected == RUN_ONCE_RUNNING) {
        // If running, spin until done
        while (atomic_load_explicit(once, memory_order_acquire) != RUN_ONCE_FINISHED) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
