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
    static IAllocator* getDefault() noexcept;

    virtual void* allocate(const size_t bytes) noexcept = 0;
    virtual void deallocate(void* ptr) noexcept = 0;
};

} //namespace lightstd
