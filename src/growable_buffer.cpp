#include "growable_buffer.h"
#include <string.h>

// -----------------------------------------------------------------------------

void gbReset(GrowableBuffer_t *gb, bool _free)
{
    if (_free) {
        if (gb->buffer) {
            free(gb->buffer);
            gb->buffer = nullptr;
        }
        gb->size = 0;
    }
    gb->used = 0;
}

void* gbReserve(GrowableBuffer_t *gb, size_t dataLen, size_t offset)
{
    if (offset > gb->used) {
        offset = gb->used;
    }
    if (dataLen > 0) {
        if (!gbEnsureSize(gb, gb->used + dataLen)) {
            return nullptr;
        }
        if (offset < gb->used) {
            memmove(gb->buffer + offset + dataLen, gb->buffer + offset, gb->used - offset);
        }
        gb->used += dataLen;
    }
    return gb->buffer + offset;
}

bool gbAdd(GrowableBuffer_t *gb, const void *data, size_t dataLen, size_t offset)
{
    void *ptr;

    if ((!data) && dataLen > 0) {
        return false;
    }

    ptr = gbReserve(gb, dataLen, offset);
    if (!ptr) {
        return false;
    }

    memcpy(ptr, data, dataLen);

    // Done
    return true;
}

void gbDel(GrowableBuffer_t *gb, size_t offset, size_t len)
{
    if (offset >= gb->used) {
        return;
    }
    if (len > gb->used - offset) {
        len = gb->used - offset;
    }
    memmove(gb->buffer + offset, gb->buffer + offset + len, gb->used - (offset + len));
    gb->used -= len;
}

bool gbEnsureSize(GrowableBuffer_t *gb, size_t size)
{
    if ((!(gb->buffer)) || size > gb->size) {
        uint8_t *newBuffer;

        size = (size + 511) & (~511);

        newBuffer = (uint8_t *)malloc(size);
        if (!newBuffer) {
            return false;
        }
        if (gb->buffer) {
            memcpy(newBuffer, gb->buffer, gb->used);
            free(gb->buffer);
        }
        gb->buffer = newBuffer;
        gb->size = size;
    }
    return true;
}

void gbWipe(GrowableBuffer_t *gb)
{
    if (gb->buffer) {
        memset(gb->buffer, 0, gb->size);
    }
}
