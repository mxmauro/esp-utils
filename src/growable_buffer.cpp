#include "growable_buffer.h"
#include <stdint.h>
#include <string.h>

// -----------------------------------------------------------------------------

void gbInit(GrowableBuffer_t* gb)
{
    gb->buffer = nullptr;
    gb->used = 0;
    gb->size = 0;
}

void gbReset(GrowableBuffer_t* gb, bool _free)
{
    if (_free)
    {
        if (gb->buffer)
        {
            free(gb->buffer);
            gb->buffer = nullptr;
        }
        gb->size = 0;
    }
    gb->used = 0;
}

void* gbReserve(GrowableBuffer_t* gb, size_t dataLen, size_t offset)
{
    if (offset > gb->used)
    {
        offset = gb->used;
    }
    if (dataLen > 0)
    {
        if (dataLen > SIZE_MAX - gb->used)
        {
            return nullptr;
        }

        if (!gbEnsureSize(gb, gb->used + dataLen))
        {
            return nullptr;
        }
        if (offset < gb->used)
        {
            memmove(gb->buffer + offset + dataLen, gb->buffer + offset, gb->used - offset);
        }
        gb->used += dataLen;
    }
    return gb->buffer ? gb->buffer + offset : nullptr;
}

bool gbAdd(GrowableBuffer_t* gb, const void* data, size_t dataLen, size_t offset)
{
    const uint8_t* source = static_cast<const uint8_t*>(data);
    uint8_t*       sourceCopy = nullptr;
    void*          ptr;

    if ((!data) && dataLen > 0)
    {
        return false;
    }
    if (dataLen == 0)
    {
        return true;
    }

    if (gb->buffer && source >= gb->buffer && source < gb->buffer + gb->used)
    {
        size_t sourceOffset = static_cast<size_t>(source - gb->buffer);

        if (dataLen > gb->used - sourceOffset)
        {
            return false;
        }

        sourceCopy = static_cast<uint8_t*>(malloc(dataLen));
        if (!sourceCopy)
        {
            return false;
        }
        memcpy(sourceCopy, source, dataLen);
        source = sourceCopy;
    }

    ptr = gbReserve(gb, dataLen, offset);
    if (!ptr)
    {
        free(sourceCopy);
        return false;
    }

    memmove(ptr, source, dataLen);
    free(sourceCopy);

    // Done
    return true;
}

void gbDel(GrowableBuffer_t* gb, size_t offset, size_t len)
{
    if (offset >= gb->used)
    {
        return;
    }
    if (len > gb->used - offset)
    {
        len = gb->used - offset;
    }
    memmove(gb->buffer + offset, gb->buffer + offset + len, gb->used - (offset + len));
    gb->used -= len;
}

bool gbEnsureSize(GrowableBuffer_t* gb, size_t size)
{
    if (size == 0)
    {
        return true;
    }

    if ((!(gb->buffer)) || size > gb->size)
    {
        uint8_t* newBuffer;

        if (size > SIZE_MAX - 511)
        {
            return false;
        }
        size = (size + 511) & (~511);

        newBuffer = static_cast<uint8_t*>(malloc(size));
        if (!newBuffer)
        {
            return false;
        }
        if (gb->buffer)
        {
            memcpy(newBuffer, gb->buffer, gb->used);
            free(gb->buffer);
        }
        gb->buffer = newBuffer;
        gb->size = size;
    }
    return true;
}

void gbWipe(GrowableBuffer_t* gb)
{
    if (gb->buffer)
    {
        memset(gb->buffer, 0, gb->size);
    }
}
