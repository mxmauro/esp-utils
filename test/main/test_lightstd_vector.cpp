#include <unity.h>

#include "lightstd/vector.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

TEST_CASE("lightstd vector resize and shrink", "lightstd vector")
{
    vector<int> v;

    TEST_ASSERT_TRUE(v.empty());
    TEST_ASSERT_TRUE(v.resize(3));
    TEST_ASSERT_EQUAL_UINT32(3, v.size());

    v[0] = 10;
    v[1] = 20;
    v[2] = 30;

    TEST_ASSERT_TRUE(v.resize(5, 7));
    TEST_ASSERT_EQUAL_UINT32(5, v.size());
    TEST_ASSERT_EQUAL(10, v[0]);
    TEST_ASSERT_EQUAL(20, v[1]);
    TEST_ASSERT_EQUAL(30, v[2]);
    TEST_ASSERT_EQUAL(7, v[3]);
    TEST_ASSERT_EQUAL(7, v[4]);

    v.resize_down(2);
    TEST_ASSERT_EQUAL_UINT32(2, v.size());

    TEST_ASSERT_TRUE(v.shrink_to_fit());
    TEST_ASSERT_EQUAL_UINT32(v.size(), v.capacity());
}
