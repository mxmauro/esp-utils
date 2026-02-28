#include <unity.h>

#include "lightstd/unordered_map.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

struct ConstantHash
{
    uint32_t operator()(const int&) const
    {
        return 1;
    }
};

// -----------------------------------------------------------------------------

TEST_CASE("lightstd static_hash_map insert update erase", "lightstd unordered_map")
{
    static_hash_map<int, int, ConstantHash> map;

    TEST_ASSERT_EQUAL(ESP_OK, map.init(8));
    TEST_ASSERT_TRUE(map.empty());

    bool inserted = false;
    int *value = map.insert(1, 100, &inserted);
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_TRUE(inserted);

    value = map.insert(2, 200, &inserted);
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_TRUE(inserted);

    value = map.insert(1, 111, &inserted);
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_FALSE(inserted);

    TEST_ASSERT_TRUE(map.contains(1));
    TEST_ASSERT_TRUE(map.contains(2));
    TEST_ASSERT_EQUAL_UINT32(2, map.size());

    TEST_ASSERT_EQUAL(111, *map.find(1));
    TEST_ASSERT_EQUAL(200, *map.find(2));

    TEST_ASSERT_TRUE(map.erase(1));
    TEST_ASSERT_FALSE(map.contains(1));
    TEST_ASSERT_TRUE(map.contains(2));

    map.done();
}
