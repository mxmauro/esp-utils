#pragma once

#include <stdatomic.h>
#include <stdint.h>

// -----------------------------------------------------------------------------

// RunOnce provides a mechanism to ensure a callback is executed only once.
typedef _Atomic(uint32_t) RunOnce_t;

typedef void (*RunOnceCallback_t)(void *ctx);

#define RUN_ONCE_INIT_STATIC 0

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void runOnceInit(RunOnce_t *once);
void runOnce(RunOnce_t *once, RunOnceCallback_t cb, void *ctx);

#ifdef __cplusplus
}
#endif // __cplusplus
