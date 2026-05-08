#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <stdatomic.h>
#include <stdint.h>

// -----------------------------------------------------------------------------

// Tracks active users so teardown can wait until the resource is no longer in use.
typedef struct RundownProtection_s {
    _Atomic(uint32_t) counter;
    _Atomic(EventGroupHandle_t) eg;
    StaticEventGroup_t eventGroupBuffer;
} RundownProtection_t;

#define RUNDOWN_PROTECTION_INIT_STATIC { \
    .counter = ATOMIC_VAR_INIT(0),       \
    .eg = ATOMIC_VAR_INIT(nullptr),      \
    .eventGroupBuffer = {}               \
}

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes a rundown protection object.
void rundownProtInit(RundownProtection_t *rp);
// Releases any resources owned by a rundown protection object.
void rundownProtDestroy(RundownProtection_t *rp);

// Acquires a usage reference unless rundown has already started.
bool rundownProtAcquire(RundownProtection_t *rp);
// Releases a previously acquired usage reference.
void rundownProtRelease(RundownProtection_t *rp);
// Starts rundown and waits for all outstanding users to leave.
void rundownProtWait(RundownProtection_t *rp);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus
class AutoRundownProtection
{
public:
    // Acquires rundown protection for the lifetime of this guard.
    AutoRundownProtection(RundownProtection_t &_rp) : rp(_rp)
    {
        wasAcquired = rundownProtAcquire(&rp);
    }
    AutoRundownProtection(const AutoRundownProtection&) = delete;
    AutoRundownProtection(AutoRundownProtection&&) = delete;

    // Releases the acquired protection when the guard goes out of scope.
    ~AutoRundownProtection()
    {
        if (wasAcquired)
        {
            rundownProtRelease(&rp);
        }
    }

    AutoRundownProtection& operator=(const AutoRundownProtection&) = delete;
    AutoRundownProtection& operator=(AutoRundownProtection&&) = delete;

    // Reports whether the guard successfully acquired protection.
    bool acquired() const
        {
            return wasAcquired;
        };

private:
    RundownProtection_t &rp;
    bool wasAcquired;
};
#endif // __cplusplus
