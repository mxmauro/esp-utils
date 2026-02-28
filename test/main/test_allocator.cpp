#include <unity.h>

#include "lightstd/allocator.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

TEST_CASE("Default allocator allocates and deallocates", "lightstd allocator")
{
    IAllocator *alloc = IAllocator::getDefault();
    TEST_ASSERT_NOT_NULL(alloc);

    void *ptr = alloc->allocate(64);
    TEST_ASSERT_NOT_NULL(ptr);

    alloc->deallocate(ptr);
}

TEST_CASE("Default allocator singleton", "getDefault returns same allocator")
{
    IAllocator *a = IAllocator::getDefault();
    IAllocator *b = IAllocator::getDefault();

    TEST_ASSERT_EQUAL_PTR(a, b);
}
