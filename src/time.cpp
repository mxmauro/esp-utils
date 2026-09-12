#include "time.h"
#include <esp_timer.h>

// -----------------------------------------------------------------------------

uint64_t now_ms(void)
{
    return static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
}
