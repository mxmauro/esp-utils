#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "onboard_led.h"

// -----------------------------------------------------------------------------

#if CONFIG_BOARD_LED_TYPE_GPIO
TEST_CASE("Onboard GPIO LED blinks three times", "onboard_led")
{
    onboardLedInit();

    for (int i = 0; i < 3; ++i)
    {
        onboardLedSet(true);
        vTaskDelay(pdMS_TO_TICKS(500));
        onboardLedSet(false);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#endif // CONFIG_BOARD_LED_TYPE_GPIO

#if CONFIG_BOARD_LED_TYPE_WS2812
TEST_CASE("Onboard WS2812 LED displays rainbow colors", "onboard_led")
{
    static const uint8_t colors[][3] = {
        {255, 0, 0},
        {255, 127, 0},
        {255, 255, 0},
        {0, 255, 0},
        {0, 0, 255},
        {75, 0, 130},
        {148, 0, 211},
    };

    onboardLedInit();

    for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i)
    {
        onboardLedSetRGB(colors[i][0], colors[i][1], colors[i][2]);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    onboardLedSet(false);
}
#endif // CONFIG_BOARD_LED_TYPE_WS2812
