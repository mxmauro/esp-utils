#pragma once

#include <stdlib.h>

#define GB_STATIC_INIT { nullptr, 0, 0 }

// -----------------------------------------------------------------------------

// Stores a dynamically sized byte buffer and its current usage.
typedef struct GrowableBuffer_s {
    uint8_t *buffer;
    size_t used;
    size_t size;
} GrowableBuffer_t;

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes a growable buffer to the empty state.
void gbInit(GrowableBuffer_t *gb);

// Resets the buffer contents and optionally frees the allocation.
void gbReset(GrowableBuffer_t *gb, bool free);

// Reserves writable space and returns a pointer to the insertion point.
void* gbReserve(GrowableBuffer_t *gb, size_t dataLen, size_t offset = (size_t)-1);
// Copies data into the buffer at the requested offset or append position.
bool gbAdd(GrowableBuffer_t *gb, const void *data, size_t dataLen, size_t offset = (size_t)-1);
// Deletes a byte range and compacts the remaining data.
void gbDel(GrowableBuffer_t *gb, size_t offset, size_t len);

// Ensures the buffer allocation can hold at least the requested size.
bool gbEnsureSize(GrowableBuffer_t *gb, size_t size);

// Zeroes the allocated buffer contents without freeing it.
void gbWipe(GrowableBuffer_t *gb);

#ifdef __cplusplus
}
#endif // __cplusplus
