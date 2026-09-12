#include <stdlib.h>
#include <unity.h>
#include "lightstd/vector.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

struct VectorLifetimeProbe
{
    static int destructorCount;
    int        value;

    explicit VectorLifetimeProbe(int _value = 0) noexcept : value(_value)
    {
    }
    VectorLifetimeProbe(const VectorLifetimeProbe&) noexcept = default;
    VectorLifetimeProbe(VectorLifetimeProbe&& other) noexcept : value(other.value)
    {
        other.value = -1;
    }
    ~VectorLifetimeProbe()
    {
        destructorCount += 1;
    }
};

int VectorLifetimeProbe::destructorCount = 0;

// -----------------------------------------------------------------------------

class FailingAllocator final : public IAllocator
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

TEST_CASE("lightstd vector resize and shrink", "lightstd vector")
{
    vector<int> v;

    TEST_ASSERT_TRUE(v.empty());
    TEST_ASSERT_EQUAL_PTR(v.begin(), v.end());
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

TEST_CASE("lightstd vector pop_back destroys once", "lightstd vector lifetime")
{
    VectorLifetimeProbe::destructorCount = 0;

    {
        vector<VectorLifetimeProbe> v;

        TEST_ASSERT_TRUE(v.emplace_back(42));
        v.pop_back();
        TEST_ASSERT_EQUAL(1, VectorLifetimeProbe::destructorCount);
    }

    TEST_ASSERT_EQUAL(1, VectorLifetimeProbe::destructorCount);
}

TEST_CASE("lightstd vector rvalue self append survives reallocation", "lightstd vector aliasing")
{
    vector<VectorLifetimeProbe> v;

    TEST_ASSERT_TRUE(v.emplace_back(42));
    TEST_ASSERT_TRUE(v.shrink_to_fit());
    TEST_ASSERT_TRUE(v.push_back(std::move(v[0])));

    TEST_ASSERT_EQUAL_UINT32(2, v.size());
    TEST_ASSERT_EQUAL(42, v[1].value);
}

TEST_CASE("lightstd vector emplace self append survives reallocation", "lightstd vector aliasing")
{
    vector<VectorLifetimeProbe> v;

    TEST_ASSERT_TRUE(v.emplace_back(42));
    TEST_ASSERT_TRUE(v.shrink_to_fit());
    TEST_ASSERT_TRUE(v.emplace_back(v[0]));

    TEST_ASSERT_EQUAL_UINT32(2, v.size());
    TEST_ASSERT_EQUAL(42, v[1].value);
}

TEST_CASE("lightstd vector rvalue self append preserves source on allocation failure", "lightstd vector aliasing")
{
    FailingAllocator            allocator;
    vector<VectorLifetimeProbe> v(&allocator);

    for (int i = 0; i < 16; ++i)
    {
        TEST_ASSERT_TRUE(v.emplace_back(i));
    }

    allocator.failAllocations = true;
    TEST_ASSERT_FALSE(v.push_back(std::move(v[0])));
    TEST_ASSERT_EQUAL_UINT32(16, v.size());
    TEST_ASSERT_EQUAL(0, v[0].value);
}

TEST_CASE("lightstd vector resize copies an aliased fill value after reallocation", "lightstd vector aliasing")
{
    vector<VectorLifetimeProbe> v;

    TEST_ASSERT_TRUE(v.emplace_back(42));
    TEST_ASSERT_TRUE(v.shrink_to_fit());
    TEST_ASSERT_TRUE(v.resize(3, v[0]));

    TEST_ASSERT_EQUAL_UINT32(3, v.size());
    TEST_ASSERT_EQUAL(42, v[0].value);
    TEST_ASSERT_EQUAL(42, v[1].value);
    TEST_ASSERT_EQUAL(42, v[2].value);
}

TEST_CASE("lightstd vector resize preserves aliased fill value on allocation failure", "lightstd vector aliasing")
{
    FailingAllocator            allocator;
    vector<VectorLifetimeProbe> v(&allocator);

    for (int i = 0; i < 16; ++i)
    {
        TEST_ASSERT_TRUE(v.emplace_back(i));
    }

    allocator.failAllocations = true;
    TEST_ASSERT_FALSE(v.resize(17, v[0]));
    TEST_ASSERT_EQUAL_UINT32(16, v.size());
    TEST_ASSERT_EQUAL(0, v[0].value);
}
