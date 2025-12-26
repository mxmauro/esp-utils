#pragma once

#include <stdlib.h>

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

namespace lightstd {

class IAllocator
{
public:
    static IAllocator* get_default() noexcept;

    virtual void* allocate(const size_t bytes) noexcept = 0;
    virtual void deallocate(void* ptr) noexcept = 0;
};

} //namespace lightstd
