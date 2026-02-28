#pragma once

#include <stdint.h>
#include <stdlib.h>

#define FNV1A32_INITIAL_HASH 2166136261u

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// FNV-1a 32-bit hash function
uint32_t fnv1a32(const void *data, size_t len, uint32_t initialHash = FNV1A32_INITIAL_HASH);

#ifdef __cplusplus
}
#endif // __cplusplus
