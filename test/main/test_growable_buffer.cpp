#include <string.h>
#include <utility>
#include <unity.h>
#include "growable_buffer.h"

// -----------------------------------------------------------------------------

TEST_CASE("GrowableBuffer add insert and delete", "buffer content management")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;
    const char*      base = "abc";
    const char*      ins = "XY";

    TEST_ASSERT_TRUE(gbAdd(&gb, base, 3));
    TEST_ASSERT_EQUAL_UINT32(3, gb.used);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)"abc", gb.buffer, 3);

    TEST_ASSERT_TRUE(gbAdd(&gb, ins, 2, 1));
    TEST_ASSERT_EQUAL_UINT32(5, gb.used);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)"aXYbc", gb.buffer, 5);

    gbDel(&gb, 2, 2);
    TEST_ASSERT_EQUAL_UINT32(3, gb.used);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)"aXc", gb.buffer, 3);

    gbReset(&gb, true);
}

TEST_CASE("GrowableBuffer reserve and wipe", "reserve appends and wipe zeroes allocated block")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;

    uint8_t* p = static_cast<uint8_t*>(gbReserve(&gb, 4, 99));
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT32(4, gb.used);

    p[0] = 0xAA;
    p[1] = 0xBB;
    p[2] = 0xCC;
    p[3] = 0xDD;

    gbWipe(&gb);

    for (size_t i = 0; i < gb.size; ++i)
    {
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

TEST_CASE("GrowableBuffer adds an aliased range", "self insertion remains valid")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;

    TEST_ASSERT_TRUE(gbAdd(&gb, "abc", 3));
    TEST_ASSERT_TRUE(gbAdd(&gb, gb.buffer, 3, 1));
    TEST_ASSERT_EQUAL_UINT32(6, gb.used);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)"aabcbc", gb.buffer, gb.used);

    gbReset(&gb, true);
}

TEST_CASE("GrowableBuffer adds an aliased range across reallocation", "self insertion remains valid")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;
    uint8_t          initial[512];

    for (size_t i = 0; i < sizeof(initial); i++)
    {
        initial[i] = static_cast<uint8_t>(i);
    }

    TEST_ASSERT_TRUE(gbAdd(&gb, initial, sizeof(initial)));
    TEST_ASSERT_TRUE(gbAdd(&gb, gb.buffer + 100, 200, 50));
    TEST_ASSERT_EQUAL_UINT32(712, gb.used);

    for (size_t i = 0; i < 50; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(initial[i], gb.buffer[i]);
    }
    for (size_t i = 0; i < 200; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(initial[i + 100], gb.buffer[i + 50]);
    }
    for (size_t i = 50; i < sizeof(initial); i++)
    {
        TEST_ASSERT_EQUAL_UINT8(initial[i], gb.buffer[i + 200]);
    }

    gbReset(&gb, true);
}

TEST_CASE("GrowableBuffer rejects overflowing capacity", "bounds")
{
    GrowableBuffer_t gb = GB_STATIC_INIT;

    TEST_ASSERT_FALSE(gbEnsureSize(&gb, SIZE_MAX));
}

TEST_CASE("GrowableBuffer C++ wrapper owns and moves allocation", "buffer C++ wrapper")
{
    GrowableBuffer buffer;

    TEST_ASSERT_TRUE(buffer.Add("abc", 3));
    TEST_ASSERT_EQUAL_UINT32(3, buffer.Size());

    GrowableBuffer moved(std::move(buffer));
    TEST_ASSERT_TRUE(buffer.Empty());
    TEST_ASSERT_EQUAL_UINT32(3, moved.Size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)"abc", moved.Data(), moved.Size());

    moved.Reset(false);
    TEST_ASSERT_TRUE(moved.Empty());
    TEST_ASSERT_TRUE(moved.Capacity() >= 3);
}
