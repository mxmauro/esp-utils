#include "rundown_protection.h"
#include <string.h>

#define RUNDOWN_IS_ACTIVE 0x80000000UL

// -----------------------------------------------------------------------------

void rundownProtInit(RundownProtection_t *rp)
{
    rp->counter.store(0);
    rp->eg = xEventGroupCreateStatic(&(rp->eventGroupBuffer));
}

void rundownProtDestroy(RundownProtection_t *rp)
{
    if (rp->eg != nullptr) {
        vEventGroupDelete(rp->eg);
        rp->eg = nullptr;
        memset(&rp->eventGroupBuffer, 0, sizeof(rp->eventGroupBuffer));
    }
    rp->counter.store(0);
}

bool rundownProtAcquire(RundownProtection_t *rp)
{
    uint32_t val;

    for (;;) {
        val = rp->counter.load();

        // If a rundown is in progress, cancel acquisition
        if (val & RUNDOWN_IS_ACTIVE) {
            return false;
        }

        // Try to increment the reference counter
        if (rp->counter.compare_exchange_strong(val, val+1)) {
            return true;
        }
    }
}

void rundownProtRelease(RundownProtection_t *rp)
{
    uint32_t val, newVal;

    for (;;) {
        // Decrement usage counter but keep the rundown active flag if present
        val = rp->counter.load();

        newVal = (val & RUNDOWN_IS_ACTIVE) | ((val & (~RUNDOWN_IS_ACTIVE)) - 1);

        if (rp->counter.compare_exchange_strong(val, newVal)) {
            // If a wait is in progress and the last reference is being released, complete the wait
            if (newVal == RUNDOWN_IS_ACTIVE) {
                xEventGroupSetBits(rp->eg, 1);
            }
            break;
        }
    }
}

void rundownProtWait(RundownProtection_t *rp)
{
    uint32_t val;

    for (;;) {
        // Get rundown active flag
        val = rp->counter.load();

        // Check if wait was already called (concurrent waits are also allowed)
        if (val & RUNDOWN_IS_ACTIVE) {
            // Wait until rundown completes
            xEventGroupWaitBits(rp->eg, 1, pdFALSE, pdTRUE, portMAX_DELAY);

            // Done
            break;
        }

        // Set rundown active flag
        if (rp->counter.compare_exchange_strong(val, val | RUNDOWN_IS_ACTIVE)) {
            // If a reference is still being held, wait until released
            if (val != 0) {
                // Wait until rundown completes
                xEventGroupWaitBits(rp->eg, 1, pdFALSE, pdTRUE, portMAX_DELAY);
            }
            else {
                xEventGroupSetBits(rp->eg, 1);
            }

            // Done
            break;
        }
    }
}
