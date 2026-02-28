#include <string.h>
#include <unity.h>

#include "growable_buffer.h"

// -----------------------------------------------------------------------------

TEST_CASE("GrowableBuffer add insert and delete", "buffer content management")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;
    const char *base = "abc";
    const char *ins = "XY";

    TEST_ASSERT_TRUE(gbAdd(&gb, base, 3));
    TEST_ASSERT_EQUAL_UINT32(3, gb.used);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)"abc", gb.buffer, 3);

    TEST_ASSERT_TRUE(gbAdd(&gb, ins, 2, 1));
    TEST_ASSERT_EQUAL_UINT32(5, gb.used);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)"aXYbc", gb.buffer, 5);

    gbDel(&gb, 2, 2);
    TEST_ASSERT_EQUAL_UINT32(3, gb.used);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)"aXc", gb.buffer, 3);

    gbReset(&gb, true);
}

TEST_CASE("GrowableBuffer reserve and wipe", "reserve appends and wipe zeroes allocated block")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;

    uint8_t *p = (uint8_t *)gbReserve(&gb, 4, 99);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT32(4, gb.used);

    p[0] = 0xAA;
    p[1] = 0xBB;
    p[2] = 0xCC;
    p[3] = 0xDD;

    gbWipe(&gb);

    for (size_t i = 0; i < gb.size; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0, gb.buffer[i]);
    }

    gbReset(&gb, true);
}

TEST_CASE("GrowableBuffer reset without free", "used resets but allocation remains")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;

    TEST_ASSERT_TRUE(gbEnsureSize(&gb, 1));
    TEST_ASSERT_TRUE(gb.size >= 512);
    TEST_ASSERT_NOT_NULL(gb.buffer);

    gb.used = 123;
    gbReset(&gb, false);

    TEST_ASSERT_EQUAL_UINT32(0, gb.used);
    TEST_ASSERT_NOT_NULL(gb.buffer);
    TEST_ASSERT_TRUE(gb.size >= 512);

    gbReset(&gb, true);
    TEST_ASSERT_NULL(gb.buffer);
    TEST_ASSERT_EQUAL_UINT32(0, gb.size);
}
