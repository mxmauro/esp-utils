#pragma once

#include <stdlib.h>

#define GB_STATIC_INIT { nullptr, 0, 0 }

#define gbInit(gb) gb = GB_STATIC_INIT

// -----------------------------------------------------------------------------

// Growable buffer is a simple structure to manage a dynamically growing buffer.
// It keeps track of used size and total allocated size.
typedef struct GrowableBuffer_s {
    uint8_t *buffer;
    size_t used;
    size_t size;
} GrowableBuffer_t;

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// If _free is true, the internal buffer is freed. Else, only the used field is reset.
void gbReset(GrowableBuffer_t *gb, bool free);

void* gbReserve(GrowableBuffer_t *gb, size_t dataLen, size_t offset = (size_t)-1);
bool gbAdd(GrowableBuffer_t *gb, const void *data, size_t dataLen, size_t offset = (size_t)-1);
void gbDel(GrowableBuffer_t *gb, size_t offset, size_t len);

bool gbEnsureSize(GrowableBuffer_t *gb, size_t size);

void gbWipe(GrowableBuffer_t *gb);

#ifdef __cplusplus
}
#endif // __cplusplus
