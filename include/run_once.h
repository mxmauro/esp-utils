#pragma once

#include <atomic>
#include <stdint.h>

// -----------------------------------------------------------------------------

// RunOnce provides a mechanism to ensure a callback is executed only once.
typedef std::atomic_uint32_t RunOnce_t;

typedef void (*RunOnceCallback_t)(const void *arg);

#define RUN_ONCE_INIT_STATIC 0

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void runOnceInit(RunOnce_t *once);
void runOnce(RunOnce_t *once, RunOnceCallback_t cb, const void *arg);

#ifdef __cplusplus
}
#endif // __cplusplus
