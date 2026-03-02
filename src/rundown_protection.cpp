#include "rundown_protection.h"
#include <assert.h>
#include <string.h>

#define RUNDOWN_IS_ACTIVE 0x80000000UL
#define RUNDOWN_REF_MASK  (~RUNDOWN_IS_ACTIVE)
#define RUNDOWN_DONE_BIT  1u

// -----------------------------------------------------------------------------

static inline EventGroupHandle_t rpEnsureEventGroup(RundownProtection_t *rp);

// -----------------------------------------------------------------------------

void rundownProtInit(RundownProtection_t *rp)
{
    EventGroupHandle_t eg;

    eg = xEventGroupCreateStatic(&rp->eventGroupBuffer);
    atomic_store_explicit(&rp->eg, eg, memory_order_release);
    atomic_store_explicit(&rp->counter, 0, memory_order_relaxed);
}

void rundownProtDestroy(RundownProtection_t *rp)
{
    EventGroupHandle_t eg;

    eg = atomic_exchange_explicit(&rp->eg, nullptr, memory_order_acq_rel);
    if (eg) {
        vEventGroupDelete(eg);
    }
    atomic_store_explicit(&rp->counter, 0, memory_order_relaxed);
    memset(&rp->eventGroupBuffer, 0, sizeof(rp->eventGroupBuffer));
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
                EventGroupHandle_t eg = atomic_load_explicit(&rp->eg, memory_order_acquire);
                xEventGroupSetBits(eg, RUNDOWN_DONE_BIT);
            }
            break;
        }
    }
}

void rundownProtWait(RundownProtection_t *rp)
{
    EventGroupHandle_t eg;
    uint32_t val, desired, isActive;

    // Lazy initialization of the event group.
    eg = rpEnsureEventGroup(rp);

    // Get rundown active flag
    val = atomic_load_explicit(&rp->counter, memory_order_acquire);
    for (;;) {
        isActive = val & RUNDOWN_IS_ACTIVE;

        // Check if wait was already called (concurrent waits are also allowed)
        if (isActive) {
            // Wait until rundown completes
            xEventGroupWaitBits(eg, RUNDOWN_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

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
                xEventGroupWaitBits(eg, RUNDOWN_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
            }
            else {
                xEventGroupSetBits(eg, RUNDOWN_DONE_BIT);
            }

            // Done
            return;
        }
    }
}

// -----------------------------------------------------------------------------

static inline EventGroupHandle_t rpEnsureEventGroup(RundownProtection_t *rp)
{
    // Choose an invalid handle value as a sentinel.
    // FreeRTOS handles are aligned pointers; (EventGroupHandle_t)1 should never be valid.
    static const EventGroupHandle_t CREATING = (EventGroupHandle_t)(uintptr_t)1;
    EventGroupHandle_t eg;
    EventGroupHandle_t expected;

    eg = atomic_load_explicit(&rp->eg, memory_order_acquire);
    if (eg && eg != CREATING) {
        return eg;
    }

    // Try to become the creator
    expected = nullptr;
    if (atomic_compare_exchange_strong_explicit(&rp->eg, &expected, CREATING,
                                                memory_order_acq_rel, memory_order_acquire)) {
        // We won: create using the per-object static buffer
        eg = xEventGroupCreateStatic(&rp->eventGroupBuffer);
        atomic_store_explicit(&rp->eg, eg, memory_order_release);
        return eg;
    }

    // Someone else is creating (or already created). Wait until it becomes non-sentinel.
    for (;;) {
        eg = atomic_load_explicit(&rp->eg, memory_order_acquire);
        if (eg && eg != CREATING) {
            return eg;
        }
        taskYIELD();
    }
}
