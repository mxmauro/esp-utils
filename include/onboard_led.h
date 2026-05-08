#pragma once

#include <driver/gpio.h>

// -----------------------------------------------------------------------------

// Configures and controls a single onboard status LED.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes the onboard LED GPIO pin.
void onboardLedInit(gpio_num_t gpioPin);
// Sets the onboard LED output state.
void onboardLedSet(bool on);

#ifdef __cplusplus
}
#endif // __cplusplus
