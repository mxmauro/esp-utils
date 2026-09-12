#pragma once

#include <stdint.h>
#include <stdlib.h>

#define FNV1A32_INITIAL_HASH 2166136261u
#define FNV1A64_INITIAL_HASH 14695981039346656037ull

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Computes a 32-bit FNV-1a hash for a byte sequence.
uint32_t fnv1a32(const void* data, size_t len, uint32_t initialHash = FNV1A32_INITIAL_HASH);

// Computes a 64-bit FNV-1a hash for a byte sequence.
uint64_t fnv1a64(const void* data, size_t len, uint64_t initialHash = FNV1A64_INITIAL_HASH);

#ifdef __cplusplus
}
#endif // __cplusplus
