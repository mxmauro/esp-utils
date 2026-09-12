#include "onboard_led.h"

#include <freertos/FreeRTOS.h>

// -----------------------------------------------------------------------------

static gpio_num_t   ledGpioPin = GPIO_NUM_NC;
static bool         active = false;
static portMUX_TYPE ledLock = portMUX_INITIALIZER_UNLOCKED;

// -----------------------------------------------------------------------------

void onboardLedInit(gpio_num_t gpioPin)
{
    assert(gpioPin != GPIO_NUM_NC);

    ledGpioPin = gpioPin;
    ESP_ERROR_CHECK(gpio_set_direction(ledGpioPin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(ledGpioPin, GPIO_FLOATING));

    // Turn the led off at startup.
    // ESP32 seems to use an inverted logic for onboard leds.
    gpio_set_level(ledGpioPin, 1);
}

void onboardLedSet(bool on)
{
    for (;;)
    {
        gpio_num_t gpioPin;

        portENTER_CRITICAL(&ledLock);
        if (on == active)
        {
            portEXIT_CRITICAL(&ledLock);
            return;
        }

        active = on;
        gpioPin = ledGpioPin;
        portEXIT_CRITICAL(&ledLock);

        // ESP32 seems to use an inverted logic for onboard leds.
        gpio_set_level(gpioPin, (on) ? 0 : 1);

        // A concurrent caller may have changed the requested state while the
        // GPIO write was in progress. Retry so the final output matches it.
        portENTER_CRITICAL(&ledLock);
        bool changed = active != on;
        portEXIT_CRITICAL(&ledLock);

        if (!changed)
        {
            return;
        }
    }
}
