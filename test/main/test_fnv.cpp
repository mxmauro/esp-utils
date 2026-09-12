#include <unity.h>
#include "fnv.h"

// -----------------------------------------------------------------------------

TEST_CASE("FNV32 known vectors", "fnv1a32 reference values")
{
    TEST_ASSERT_EQUAL_HEX32(0x811C9DC5u, fnv1a32("", 0, FNV1A32_INITIAL_HASH));
    TEST_ASSERT_EQUAL_HEX32(0xE40C292Cu, fnv1a32("a", 1, FNV1A32_INITIAL_HASH));
    TEST_ASSERT_EQUAL_HEX32(0xBF9CF968u, fnv1a32("foobar", 6, FNV1A32_INITIAL_HASH));
}

TEST_CASE("FNV32 incremental hashing", "split input equals single-pass hash")
{
    const char* left = "foo";
    const char* right = "bar";

    uint32_t h = fnv1a32(left, 3, FNV1A32_INITIAL_HASH);
    h = fnv1a32(right, 3, h);

    TEST_ASSERT_EQUAL_HEX32(fnv1a32("foobar", 6, FNV1A32_INITIAL_HASH), h);
}

TEST_CASE("FNV64 known vectors", "fnv1a64 reference values")
{
    TEST_ASSERT_EQUAL_HEX64(0xCBF29CE484222325ull, fnv1a64("", 0, FNV1A64_INITIAL_HASH));
    TEST_ASSERT_EQUAL_HEX64(0xAF63DC4C8601EC8Cull, fnv1a64("a", 1, FNV1A64_INITIAL_HASH));
    TEST_ASSERT_EQUAL_HEX64(0x85944171F73967E8ull, fnv1a64("foobar", 6, FNV1A64_INITIAL_HASH));
}

TEST_CASE("FNV64 incremental hashing", "split input equals single-pass hash")
{
    const char* left = "foo";
    const char* right = "bar";

    uint64_t h = fnv1a64(left, 3, FNV1A64_INITIAL_HASH);
    h = fnv1a64(right, 3, h);

    TEST_ASSERT_EQUAL_HEX64(fnv1a64("foobar", 6, FNV1A64_INITIAL_HASH), h);
}
