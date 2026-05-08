#pragma once

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include <stdlib.h>

// -----------------------------------------------------------------------------

namespace lightstd {

class IAllocator
{
public:
    // Returns the process-wide default allocator implementation.
    static IAllocator* getDefault() noexcept;

    // Allocates an uninitialized block of the requested size.
    virtual void* allocate(const size_t bytes) noexcept = 0;
    // Releases a block previously returned by allocate().
    virtual void deallocate(void* ptr) noexcept = 0;
};

} //namespace lightstd
