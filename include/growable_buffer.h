#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define GB_STATIC_INIT {NULL, 0, 0}

// -----------------------------------------------------------------------------

// Stores a dynamically sized byte buffer and its current usage.
typedef struct GrowableBuffer_s {
    uint8_t* buffer;
    size_t   used;
    size_t   size;
} GrowableBuffer_t;

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes a growable buffer to the empty state.
void gbInit(GrowableBuffer_t* gb);

// Resets the buffer contents and optionally frees the allocation.
void gbReset(GrowableBuffer_t* gb, bool free);

// Reserves writable space and returns a pointer to the insertion point.
#ifdef __cplusplus
void* gbReserve(GrowableBuffer_t* gb, size_t dataLen, size_t offset = SIZE_MAX);
#else
void* gbReserve(GrowableBuffer_t* gb, size_t dataLen, size_t offset);
#endif // __cplusplus
// Copies data into the buffer at the requested offset or append position.
#ifdef __cplusplus
bool gbAdd(GrowableBuffer_t* gb, const void* data, size_t dataLen, size_t offset = SIZE_MAX);
#else
bool gbAdd(GrowableBuffer_t* gb, const void* data, size_t dataLen, size_t offset);
#endif // __cplusplus
// Deletes a byte range and compacts the remaining data.
void gbDel(GrowableBuffer_t* gb, size_t offset, size_t len);

// Ensures the buffer allocation can hold at least the requested size.
bool gbEnsureSize(GrowableBuffer_t* gb, size_t size);

// Zeroes the allocated buffer contents without freeing it.
void gbWipe(GrowableBuffer_t* gb);

#ifdef __cplusplus
}
#endif // __cplusplus

// -----------------------------------------------------------------------------

#ifdef __cplusplus

class GrowableBuffer
{
public:
    GrowableBuffer() noexcept
    {
        gbInit(&gb);
    }
    GrowableBuffer(const GrowableBuffer&) noexcept = delete;
    GrowableBuffer(GrowableBuffer&& other) noexcept : gb(other.gb)
    {
        gbInit(&other.gb);
    }

    // Frees any allocation owned by the buffer.
    ~GrowableBuffer() noexcept
    {
        gbReset(&gb, true);
    }

    GrowableBuffer& operator=(const GrowableBuffer&) noexcept = delete;
    GrowableBuffer& operator=(GrowableBuffer&& other) noexcept
    {
        if (this != &other)
        {
            gbReset(&gb, true);
            gb = other.gb;
            gbInit(&other.gb);
        }
        return *this;
    }

    uint8_t* Data() noexcept
    {
        return gb.buffer;
    }

    const uint8_t* Data() const noexcept
    {
        return gb.buffer;
    }

    size_t Size() const noexcept
    {
        return gb.used;
    }

    size_t Capacity() const noexcept
    {
        return gb.size;
    }

    bool Empty() const noexcept
    {
        return gb.used == 0;
    }

    // Resets contents and optionally keeps the allocation for reuse.
    void Reset(bool free = true) noexcept
    {
        gbReset(&gb, free);
    }

    // Reserves writable space at offset or the current append position.
    void* Reserve(size_t dataLen, size_t offset = SIZE_MAX) noexcept
    {
        return gbReserve(&gb, dataLen, offset);
    }

    // Copies data at offset or the current append position.
    bool Add(const void* data, size_t dataLen, size_t offset = SIZE_MAX) noexcept
    {
        return gbAdd(&gb, data, dataLen, offset);
    }

    // Deletes a byte range and compacts the remaining data.
    void Delete(size_t offset, size_t len) noexcept
    {
        gbDel(&gb, offset, len);
    }

    // Ensures the backing allocation has at least the requested capacity.
    bool EnsureSize(size_t size) noexcept
    {
        return gbEnsureSize(&gb, size);
    }

    // Zeroes the complete backing allocation.
    void Wipe() noexcept
    {
        gbWipe(&gb);
    }

    // Returns the underlying C buffer without transferring ownership.
    GrowableBuffer_t& NativeHandle() noexcept
    {
        return gb;
    }

    const GrowableBuffer_t& NativeHandle() const noexcept
    {
        return gb;
    }

private:
    GrowableBuffer_t gb;
};

#endif // __cplusplus
