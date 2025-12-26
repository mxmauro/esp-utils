#pragma once

#include <driver/gpio.h>

// -----------------------------------------------------------------------------

// Onboard LED control functions

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void onboardLedInit(gpio_num_t gpioPin);
void onboardLedSet(bool on);

#ifdef __cplusplus
}
#endif // __cplusplus
