#include "rundown_protection.h"
#include <assert.h>
#include <string.h>

#define RUNDOWN_IS_ACTIVE 0x80000000UL
#define RUNDOWN_REF_MASK  (~RUNDOWN_IS_ACTIVE)
#define RUNDOWN_DONE_BIT  1u

// -----------------------------------------------------------------------------

void rundownProtInit(RundownProtection_t *rp)
{
    atomic_store_explicit(&rp->counter, 0, memory_order_relaxed);
    rp->eg = xEventGroupCreateStatic(&(rp->eventGroupBuffer));
}

void rundownProtDestroy(RundownProtection_t *rp)
{
    if (rp->eg != nullptr) {
        vEventGroupDelete(rp->eg);
        rp->eg = nullptr;
        memset(&rp->eventGroupBuffer, 0, sizeof(rp->eventGroupBuffer));
    }
    atomic_store_explicit(&rp->counter, 0, memory_order_relaxed);
}

bool rundownProtAcquire(RundownProtection_t *rp)
{
    uint32_t val, desired;

    val = atomic_load_explicit(&rp->counter, memory_order_acquire);
    for (;;) {
        // If a rundown is in progress, cancel acquisition
        if (val & RUNDOWN_IS_ACTIVE) {
            return false;
        }
        if ((val & RUNDOWN_REF_MASK) == RUNDOWN_REF_MASK) {
            return false; // overflow guard
        }

        // Try to increment the reference counter
        desired = val + 1;
        if (atomic_compare_exchange_strong_explicit(&rp->counter, &val, desired,
                                                    memory_order_acquire, memory_order_acquire))
        {
            return true;
        }
    }
}

void rundownProtRelease(RundownProtection_t *rp)
{
    uint32_t val, desired, ref;

    val = atomic_load_explicit(&rp->counter, memory_order_acquire);
    for (;;) {
        ref = val & RUNDOWN_REF_MASK;
        if (ref == 0) {
            assert(false); // assert / error: release without acquire
            return;
        }

        // Decrement usage counter but keep the rundown active flag if present
        desired  = (val & RUNDOWN_IS_ACTIVE) | (ref - 1);

        if (atomic_compare_exchange_strong_explicit(&rp->counter, &val, desired,
                                                    memory_order_release, memory_order_acquire))
        {
            // If a wait is in progress and the last reference is being released, complete the wait
            if (desired == RUNDOWN_IS_ACTIVE) {
                xEventGroupSetBits(rp->eg, RUNDOWN_DONE_BIT);
            }
            break;
        }
    }
}

void rundownProtWait(RundownProtection_t *rp)
{
    uint32_t val, desired, isActive;

    // Get rundown active flag
    val = atomic_load_explicit(&rp->counter, memory_order_acquire);
    for (;;) {
        isActive = val & RUNDOWN_IS_ACTIVE;

        // Check if wait was already called (concurrent waits are also allowed)
        if (isActive) {
            // Wait until rundown completes
            xEventGroupWaitBits(rp->eg, RUNDOWN_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

            // Done
            return;
        }

        // Set rundown active flag
        desired = val | RUNDOWN_IS_ACTIVE;
        if (atomic_compare_exchange_strong_explicit(&rp->counter, &val, desired,
                                                    memory_order_acq_rel, memory_order_acquire))
        {
            // If a reference is still being held, wait until released
            if (isActive) {
                // Wait until rundown completes
                xEventGroupWaitBits(rp->eg, RUNDOWN_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
            }
            else {
                xEventGroupSetBits(rp->eg, RUNDOWN_DONE_BIT);
            }

            // Done
            return;
        }
    }
}
