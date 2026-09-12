#include <stdlib.h>
#include <unity.h>
#include "lightstd/string.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

class StringFailingAllocator final : public IAllocator
{
public:
    bool failAllocations{false};

    void* allocate(const size_t bytes) noexcept override
    {
        return failAllocations ? nullptr : malloc(bytes);
    }

    void deallocate(void* ptr) noexcept override
    {
        free(ptr);
    }
};

// -----------------------------------------------------------------------------

TEST_CASE("lightstd string append and resize", "lightstd string")
{
    string s;

    TEST_ASSERT_TRUE(s.empty());
    TEST_ASSERT_EQUAL_PTR(s.begin(), s.end());
    TEST_ASSERT_TRUE(s.append("ab"));
    TEST_ASSERT_TRUE(s.append("cdef", 2));
    TEST_ASSERT_EQUAL_UINT32(4, s.length());
    TEST_ASSERT_EQUAL_STRING("abcd", s.c_str());

    TEST_ASSERT_TRUE(s.resize(2));
    TEST_ASSERT_EQUAL_STRING("ab", s.c_str());

    s.clear();
    TEST_ASSERT_TRUE(s.empty());
    TEST_ASSERT_EQUAL_STRING("", s.c_str());
}

TEST_CASE("lightstd string appends itself", "lightstd string aliasing")
{
    string s;

    TEST_ASSERT_TRUE(s.append("ab"));
    TEST_ASSERT_TRUE(s.append(s.c_str()));
    TEST_ASSERT_EQUAL_STRING("abab", s.c_str());
}

TEST_CASE("lightstd string appends an overlapping suffix across reallocation", "lightstd string aliasing")
{
    string s;

    TEST_ASSERT_TRUE(s.append("abc"));
    TEST_ASSERT_TRUE(s.append(s.c_str() + 1, 2));
    TEST_ASSERT_EQUAL_STRING("abcbc", s.c_str());
}

TEST_CASE("lightstd string preserves content when append allocation fails", "lightstd string allocation failure")
{
    StringFailingAllocator allocator;
    string                 s(&allocator);

    TEST_ASSERT_TRUE(s.append("abc"));
    allocator.failAllocations = true;
    TEST_ASSERT_FALSE(s.append("defg"));
    TEST_ASSERT_EQUAL_STRING("abc", s.c_str());
}

TEST_CASE("lightstd string rejects overflowing capacity", "lightstd string bounds")
{
    string s;

    TEST_ASSERT_FALSE(s.reserve(SIZE_MAX));
}
