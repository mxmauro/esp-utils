#pragma once

#include <stdatomic.h>
#include <stdint.h>

// -----------------------------------------------------------------------------

// Tracks whether a one-time callback has already executed.
typedef _Atomic(uint32_t) RunOnce_t;

typedef void (*RunOnceCallback_t)(void *ctx);

#define RUN_ONCE_INIT_STATIC 0

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes a run-once state object before first use.
void runOnceInit(RunOnce_t *once);
// Executes the callback only on the first successful invocation.
void runOnce(RunOnce_t *once, RunOnceCallback_t cb, void *ctx);

#ifdef __cplusplus
}
#endif // __cplusplus
