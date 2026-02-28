#include <unity.h>

#include "lightstd/string.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

TEST_CASE("lightstd string append and resize", "lightstd string")
{
    string s;

    TEST_ASSERT_TRUE(s.empty());
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
