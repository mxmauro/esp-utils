#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdint.h>

// -----------------------------------------------------------------------------

// Tracks active users so teardown can wait until the resource is no longer in use.
typedef struct RundownProtection_s {
    _Atomic(uint32_t)           counter;
    _Atomic(uint32_t)           waiters;
    _Atomic(EventGroupHandle_t) eg;
    StaticEventGroup_t          eventGroupBuffer;
} RundownProtection_t;

#ifdef __cplusplus
    #define RUNDOWN_PROTECTION_INIT_STATIC                                                                                     \
        {                                                                                                                      \
            .counter = ATOMIC_VAR_INIT(0), .waiters = ATOMIC_VAR_INIT(0), .eg = ATOMIC_VAR_INIT(NULL), .eventGroupBuffer = {}  \
        }
#else
    #define RUNDOWN_PROTECTION_INIT_STATIC                                                                                     \
        {                                                                                                                      \
            .counter = ATOMIC_VAR_INIT(0), .waiters = ATOMIC_VAR_INIT(0), .eg = ATOMIC_VAR_INIT(NULL), .eventGroupBuffer = {0} \
        }
#endif // __cplusplus

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes a rundown protection object.
void rundownProtInit(RundownProtection_t* rp);
// Releases resources after rundownProtWait() has returned. This waits for
// concurrent rundownProtWait() callers that were already in progress. Before
// calling this, stop new callers from invoking any rundownProt* function on
// this object.
// The object must be reinitialized before it can be used again.
void rundownProtDestroy(RundownProtection_t* rp);

// Acquires a usage reference unless rundown has already started.
bool rundownProtAcquire(RundownProtection_t* rp);
// Releases a previously acquired usage reference.
void rundownProtRelease(RundownProtection_t* rp);
// Starts rundown and waits for all outstanding users to leave.
void rundownProtWait(RundownProtection_t* rp);

#ifdef __cplusplus
}
#endif // __cplusplus

// -----------------------------------------------------------------------------

#ifdef __cplusplus

class RundownProtection
{
public:
    RundownProtection() noexcept
    {
        rundownProtInit(&rp);
    }
    RundownProtection(const RundownProtection&) noexcept = delete;
    RundownProtection(RundownProtection&&) noexcept = delete;

    // Starts rundown, waits for holders, and releases the backing event group.
    ~RundownProtection() noexcept
    {
        rundownProtWait(&rp);
        rundownProtDestroy(&rp);
    }

    RundownProtection& operator=(const RundownProtection&) noexcept = delete;
    RundownProtection& operator=(RundownProtection&&) noexcept = delete;

    // Acquires a usage reference unless rundown has already started.
    bool Acquire() noexcept
    {
        return rundownProtAcquire(&rp);
    }

    // Releases a previously acquired usage reference.
    void Release() noexcept
    {
        rundownProtRelease(&rp);
    }

    // Starts rundown and waits for all outstanding users to leave.
    void Wait() noexcept
    {
        rundownProtWait(&rp);
    }

    // Returns the underlying C state without transferring ownership.
    RundownProtection_t& NativeHandle() noexcept
    {
        return rp;
    }

private:
    RundownProtection_t rp;
};

// -----------------------------------------------------------------------------

class AutoRundownProtection
{
public:
    // Acquires rundown protection for the lifetime of this guard.
    AutoRundownProtection(RundownProtection_t& _rp) noexcept : rp(&_rp), wasAcquired(rundownProtAcquire(rp))
    {
    }
    // Acquires rundown protection from its C++ owner for the lifetime of this guard.
    AutoRundownProtection(RundownProtection& _rp) noexcept : AutoRundownProtection(_rp.NativeHandle())
    {
    }
    AutoRundownProtection(const AutoRundownProtection&) noexcept = delete;
    AutoRundownProtection(AutoRundownProtection&&) noexcept = delete;

    // Releases the acquired protection when the guard goes out of scope.
    ~AutoRundownProtection() noexcept
    {
        if (wasAcquired)
        {
            rundownProtRelease(rp);
        }
    }

    AutoRundownProtection& operator=(const AutoRundownProtection&) noexcept = delete;
    AutoRundownProtection& operator=(AutoRundownProtection&&) noexcept = delete;

    // Reports whether the guard successfully acquired protection.
    bool acquired() const noexcept
    {
        return wasAcquired;
    }

private:
    RundownProtection_t* rp;
    bool                 wasAcquired;
};

#endif // __cplusplus
