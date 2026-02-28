#include <utility>
#include <unity.h>

#include "lightstd/functional.h"

using namespace lightstd;

// -----------------------------------------------------------------------------

TEST_CASE("lightstd light_function copy move and reset", "lightstd functional")
{
    light_function<int(int)> fn = [](int x) {
        return x + 5;
    };

    TEST_ASSERT_TRUE((bool)fn);
    TEST_ASSERT_EQUAL(8, fn(3));

    light_function<int(int)> copy(fn);
    TEST_ASSERT_EQUAL(9, copy(4));

    light_function<int(int)> moved(std::move(copy));
    TEST_ASSERT_TRUE((bool)moved);
    TEST_ASSERT_FALSE((bool)copy);
    TEST_ASSERT_EQUAL(10, moved(5));

    moved = nullptr;
    TEST_ASSERT_FALSE((bool)moved);
}
