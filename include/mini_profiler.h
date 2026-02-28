#pragma once


#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include <esp_log.h>
#include <esp_timer.h>
#include <stdint.h>
#include <stdio.h>

#define MINIPROFILE_CONCAT_HELPER_IMPL(x, y) x##y
#define MINIPROFILE_CONCAT_HELPER(x, y)      MINIPROFILE_CONCAT_HELPER_IMPL(x, y)

#define MINIPROFILE_FUNC(TAG)  MiniProfiler scopedMiniProfilerFunc(TAG, __func__)
#define MINIPROFILE(TAG, name) MiniProfiler MINIPROFILE_CONCAT_HELPER(scopedMiniProfiler, __LINE__)(TAG, name)

// -----------------------------------------------------------------------------

// MiniProfiler implements a simple RAII-style profiler that logs the start time and duration
// of a code block.
class MiniProfiler
{
public:
    explicit MiniProfiler(const char *_tag, const char* _name) : tag(_tag), name(_name), startTimeUs(esp_timer_get_time())
    {
    }
    MiniProfiler(const MiniProfiler&) = delete;
    MiniProfiler(MiniProfiler&&) = delete;

    ~MiniProfiler()
    {
        char startTimeBuf[64];
        char durationBuf[64];

        uint64_t now = esp_timer_get_time();

        formatTime(startTimeUs, startTimeBuf);
        formatDuration(now - startTimeUs, durationBuf);
        ESP_LOGD(tag, "%s | Start-Time: %s (Elapsed: %s)", name, startTimeBuf, durationBuf);
    }

    MiniProfiler& operator=(const MiniProfiler&) = delete;
    MiniProfiler& operator=(MiniProfiler&&) = delete;

    static void formatTime(uint64_t t, char buf[64])
    {
        uint64_t sec = t / 1'000'000;
        uint64_t usec = t % 1'000'000;
        snprintf(buf, 64, "%llu:%02u:%02u.%06llu", (sec / 3600), (unsigned int)(sec / 60) % 60, (unsigned int)(sec % 60), usec);
    }

    static void formatDuration(uint64_t d, char buf[64])
    {
        if (d < 1000)
            snprintf(buf, 64, "%llu µs", d);
        else if (d < 1'000'000)
            snprintf(buf, 64, "%.3f ms", d / 1000.0);
        else
            snprintf(buf, 64, "%.3f s", d / 1'000'000.0);
    }

private:
    const char *tag;
    const char *name;
    uint64_t startTimeUs;
};
