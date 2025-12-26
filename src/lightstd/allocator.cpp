#include "lightstd/allocator.h"
#include <memory.h>

// -----------------------------------------------------------------------------

namespace lightstd {

class DefaultAllocator : public IAllocator
{
public:
    void* allocate(const size_t bytes) noexcept
    {
        return malloc(bytes);
    }

    void deallocate(void* ptr) noexcept
    {
        free(ptr);
    }
};

IAllocator* IAllocator::get_default() noexcept
{
    static DefaultAllocator alloc;

    return &alloc;
}

} //namespace lightstd
