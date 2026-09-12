#include <unity.h>
#include "mini_profiler.h"

// -----------------------------------------------------------------------------

TEST_CASE("MiniProfiler formats absolute timestamps", "mini profiler")
{
    char formatted[64];

    MiniProfiler::formatTime(3600000001ULL, formatted);
    TEST_ASSERT_EQUAL_STRING("1:00:00.000001", formatted);
}

TEST_CASE("MiniProfiler formats duration units", "mini profiler")
{
    char formatted[64];

    MiniProfiler::formatDuration(999, formatted);
    TEST_ASSERT_EQUAL_STRING("999 µs", formatted);
    MiniProfiler::formatDuration(1234, formatted);
    TEST_ASSERT_EQUAL_STRING("1.234 ms", formatted);
    MiniProfiler::formatDuration(1500000, formatted);
    TEST_ASSERT_EQUAL_STRING("1.500 s", formatted);
}
