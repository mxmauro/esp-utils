#include <unity.h>
#include "lightstd/unordered_map.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

struct ConstantHash
{
    uint32_t operator()(const int&) const noexcept
    {
        return 1;
    }
};

// -----------------------------------------------------------------------------

struct LifetimeKey
{
    int value;

    bool operator==(const LifetimeKey& other) const noexcept
    {
        return value == other.value;
    }
};

struct LifetimeValue
{
    static int liveInstances;
    int        value;

    explicit LifetimeValue(int _value = 0) noexcept : value(_value)
    {
        liveInstances += 1;
    }
    LifetimeValue(const LifetimeValue& other) noexcept : value(other.value)
    {
        liveInstances += 1;
    }
    LifetimeValue& operator=(const LifetimeValue& other) noexcept
    {
        value = other.value;
        return *this;
    }
    ~LifetimeValue()
    {
        liveInstances -= 1;
    }
};

int LifetimeValue::liveInstances = 0;

struct LifetimeHash
{
    uint32_t operator()(const LifetimeKey& key) const noexcept
    {
        return static_cast<uint32_t>(key.value);
    }
};

// -----------------------------------------------------------------------------

TEST_CASE("lightstd static_hash_map insert update erase", "lightstd unordered_map")
{
    static_hash_map<int, int, ConstantHash> map;

    TEST_ASSERT_EQUAL(ESP_OK, map.init(8));
    TEST_ASSERT_TRUE(map.empty());

    bool inserted = false;
    int* value = map.insert(1, 100, &inserted);
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

TEST_CASE("lightstd static_hash_map owns non-trivial values", "lightstd unordered_map lifetime")
{
    LifetimeValue::liveInstances = 0;

    {
        static_hash_map<LifetimeKey, LifetimeValue, LifetimeHash> map;
        LifetimeKey                                               key{1};
        LifetimeValue                                             value{100};

        TEST_ASSERT_EQUAL(ESP_OK, map.init(4));
        TEST_ASSERT_NOT_NULL(map.insert(key, value));
        TEST_ASSERT_EQUAL(2, LifetimeValue::liveInstances);

        TEST_ASSERT_TRUE(map.erase(key));
        TEST_ASSERT_EQUAL(1, LifetimeValue::liveInstances);

        TEST_ASSERT_NOT_NULL(map.insert(key, value));
        TEST_ASSERT_EQUAL(2, LifetimeValue::liveInstances);
        TEST_ASSERT_EQUAL(ESP_OK, map.init(8));
        TEST_ASSERT_EQUAL(1, LifetimeValue::liveInstances);
    }

    TEST_ASSERT_EQUAL(0, LifetimeValue::liveInstances);
}

TEST_CASE("lightstd static_hash_map rejects operations before init", "lightstd unordered_map")
{
    static_hash_map<int, int, ConstantHash> map;

    TEST_ASSERT_NULL(map.find(1));
    TEST_ASSERT_FALSE(map.erase(1));
    TEST_ASSERT_NULL(map.insert(1, 1));
}

TEST_CASE("lightstd static_hash_map compacts collision tombstones in place", "lightstd unordered_map")
{
    static_hash_map<int, int, ConstantHash> map;

    TEST_ASSERT_EQUAL(ESP_OK, map.init(8));
    TEST_ASSERT_NOT_NULL(map.insert(1, 10));
    TEST_ASSERT_NOT_NULL(map.insert(2, 20));
    TEST_ASSERT_NOT_NULL(map.insert(3, 30));
    TEST_ASSERT_NOT_NULL(map.insert(4, 40));

    TEST_ASSERT_TRUE(map.erase(1));
    TEST_ASSERT_TRUE(map.erase(2));
    TEST_ASSERT_TRUE(map.erase(3));

    TEST_ASSERT_FALSE(map.contains(1));
    TEST_ASSERT_FALSE(map.contains(2));
    TEST_ASSERT_FALSE(map.contains(3));
    TEST_ASSERT_EQUAL(40, *map.find(4));
    TEST_ASSERT_NOT_NULL(map.insert(5, 50));
    TEST_ASSERT_EQUAL(50, *map.find(5));
}

TEST_CASE("lightstd static_hash_map handles full wrapped probe chains", "lightstd unordered_map boundaries")
{
    static_hash_map<int, int, ConstantHash> map;
    bool                                    inserted = false;

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, map.init(0));
    TEST_ASSERT_EQUAL(ESP_OK, map.init(3));
    TEST_ASSERT_NOT_NULL(map.insert(1, 10, &inserted));
    TEST_ASSERT_TRUE(inserted);
    TEST_ASSERT_NOT_NULL(map.insert(2, 20, &inserted));
    TEST_ASSERT_TRUE(inserted);
    TEST_ASSERT_NOT_NULL(map.insert(3, 30, &inserted));
    TEST_ASSERT_TRUE(inserted);
    TEST_ASSERT_NULL(map.insert(4, 40, &inserted));
    TEST_ASSERT_FALSE(inserted);

    TEST_ASSERT_NOT_NULL(map.insert(2, 200, &inserted));
    TEST_ASSERT_FALSE(inserted);
    TEST_ASSERT_EQUAL(200, *map.find(2));

    TEST_ASSERT_TRUE(map.erase(2));
    map.compact();
    TEST_ASSERT_EQUAL(10, *map.find(1));
    TEST_ASSERT_EQUAL(30, *map.find(3));
    TEST_ASSERT_NOT_NULL(map.insert(4, 40, &inserted));
    TEST_ASSERT_TRUE(inserted);
    TEST_ASSERT_EQUAL_UINT32(3, map.size());

    TEST_ASSERT_EQUAL(ESP_OK, map.init(8));
    TEST_ASSERT_NOT_NULL(map.insert(1, 10));
    TEST_ASSERT_NOT_NULL(map.insert(2, 20));
    TEST_ASSERT_NOT_NULL(map.insert(3, 30));
    TEST_ASSERT_TRUE(map.erase(1));
    map.compact();
    TEST_ASSERT_EQUAL(20, *map.find(2));
    TEST_ASSERT_EQUAL(30, *map.find(3));

    TEST_ASSERT_EQUAL(ESP_OK, map.init(8));
    for (int i = 0; i < 8; i++)
    {
        TEST_ASSERT_NOT_NULL(map.insert(i, i, &inserted));
        TEST_ASSERT_TRUE(inserted);
    }
    TEST_ASSERT_TRUE(map.erase(3));
    TEST_ASSERT_NOT_NULL(map.insert(8, 80, &inserted));
    TEST_ASSERT_TRUE(inserted);
    TEST_ASSERT_EQUAL(80, *map.find(8));
}

TEST_CASE("lightstd static_hash_map const iterator and empty begin", "lightstd unordered_map")
{
    static_hash_map<int, int, ConstantHash> emptyMap;
    size_t                                  iterated = 0;

    for (auto it = emptyMap.begin(); it != emptyMap.end(); ++it)
    {
        iterated += 1;
    }
    TEST_ASSERT_EQUAL_UINT32(0, iterated);

    static_hash_map<int, int, ConstantHash> map;
    TEST_ASSERT_EQUAL(ESP_OK, map.init(8));
    TEST_ASSERT_NOT_NULL(map.insert(3, 300));
    TEST_ASSERT_NOT_NULL(map.insert(7, 700));

    const static_hash_map<int, int, ConstantHash>& constMap = map;
    int                                            sum = 0;
    size_t                                         count = 0;

    for (auto it = constMap.begin(); it != constMap.end(); ++it)
    {
        auto entry = *it;

        sum += entry.key + entry.value;
        count += 1;
    }

    TEST_ASSERT_EQUAL_UINT32(2, count);
    TEST_ASSERT_EQUAL(1010, sum);

    map.done();
}
