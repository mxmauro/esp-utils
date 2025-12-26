#include "onboard_led.h"

// -----------------------------------------------------------------------------

static gpio_num_t ledGpioPin = GPIO_NUM_NC;
static bool active = false;

// -----------------------------------------------------------------------------

void onboardLedInit(gpio_num_t gpioPin)
{
    ledGpioPin = gpioPin;
    ESP_ERROR_CHECK(gpio_set_direction(ledGpioPin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(ledGpioPin, GPIO_FLOATING));

    // Turn the led off at startup.
    // ESP32 seems to use an inverted logic for onboard leds.
    gpio_set_level(ledGpioPin, 1);
}

void onboardLedSet(bool on)
{
    if (on != active) {
        active = on;

        // ESP32 seems to use an inverted logic for onboard leds.
        gpio_set_level(ledGpioPin, (on) ? 0 : 1);
    }
}
