#include <stdint.h>
#include <utility>
#include <unity.h>
#include "lightstd/functional.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

struct LargeCallable
{
    uint8_t padding[48]{};
    int     value;

    explicit LargeCallable(int _value) noexcept : value(_value)
    {
    }

    int operator()(int arg) const noexcept
    {
        return arg + value;
    }
};

// -----------------------------------------------------------------------------

TEST_CASE("lightstd light_function copy move and reset", "lightstd functional")
{
    light_function<int(int)> fn = [](int x)
    {
        return x + 5;
    };

    TEST_ASSERT_TRUE(static_cast<bool>(fn));
    TEST_ASSERT_EQUAL(8, fn(3));

    light_function<int(int)> copy(fn);
    TEST_ASSERT_EQUAL(9, copy(4));

    light_function<int(int)> moved(std::move(copy));
    TEST_ASSERT_TRUE(static_cast<bool>(moved));
    TEST_ASSERT_FALSE(static_cast<bool>(copy));
    TEST_ASSERT_EQUAL(10, moved(5));

    moved = nullptr;
    TEST_ASSERT_FALSE(static_cast<bool>(moved));
}

TEST_CASE("lightstd light_function copies and moves heap callables", "lightstd functional heap storage")
{
    static_assert(sizeof(LargeCallable) > 32, "callable must exceed the default SBO capacity");

    light_function<int(int)> fn = LargeCallable(7);
    TEST_ASSERT_TRUE(fn != nullptr);
    TEST_ASSERT_FALSE(fn == nullptr);
    TEST_ASSERT_EQUAL(10, fn(3));

    light_function<int(int)> copy(fn);
    TEST_ASSERT_EQUAL(11, copy(4));

    light_function<int(int)> moved(std::move(copy));
    TEST_ASSERT_FALSE(copy != nullptr);
    TEST_ASSERT_TRUE(nullptr != moved);
    TEST_ASSERT_EQUAL(12, moved(5));

    moved = LargeCallable(9);
    fn.swap(moved);
    TEST_ASSERT_EQUAL(14, fn(5));
    TEST_ASSERT_EQUAL(10, moved(3));
}
