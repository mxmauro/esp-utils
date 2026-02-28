#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <stdatomic.h>
#include <stdint.h>

// -----------------------------------------------------------------------------

// A rundown protection is a lightweight synchronization mechanism that prevents
// an object or resource from being destroyed while it is still in use. It lets
// your app to safely coordinate between threads that access shared state and
// code paths that may tear down that state (e.g., during unload).
typedef struct RundownProtection_s {
    _Atomic(uint32_t) counter;
    EventGroupHandle_t eg;
    StaticEventGroup_t eventGroupBuffer;
} RundownProtection_t;

#define RUNDOWN_PROTECTION_INIT_STATIC { .counter = 0, .eg = nullptr, .eventGroupBuffer = {} }

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void rundownProtInit(RundownProtection_t *rp);
void rundownProtDestroy(RundownProtection_t *rp);

// Acquire increments the usage counter unless a rundown is in progress.
bool rundownProtAcquire(RundownProtection_t *rp);
// Release decrements the usage counter.
void rundownProtRelease(RundownProtection_t *rp);
// Wait initiates the shutdown process and waits until all acquisitions are released.
void rundownProtWait(RundownProtection_t *rp);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus
class AutoRundownProtection
{
public:
    AutoRundownProtection(RundownProtection_t &_rp) : rp(_rp)
    {
        wasAcquired = rundownProtAcquire(&rp);
    }
    AutoRundownProtection(const AutoRundownProtection&) = delete;
    AutoRundownProtection(AutoRundownProtection&&) = delete;

    ~AutoRundownProtection()
    {
        if (wasAcquired)
        {
            rundownProtRelease(&rp);
        }
    }

    AutoRundownProtection& operator=(const AutoRundownProtection&) = delete;
    AutoRundownProtection& operator=(AutoRundownProtection&&) = delete;

    bool acquired() const
        {
            return wasAcquired;
        };

private:
    RundownProtection_t &rp;
    bool wasAcquired;
};
#endif // __cplusplus
