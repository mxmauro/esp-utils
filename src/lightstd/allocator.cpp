#include "lightstd/allocator.h"
#include <memory.h>

using namespace lightstd;

// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------

IAllocator* IAllocator::getDefault() noexcept
{
    static DefaultAllocator alloc;

    return &alloc;
}
