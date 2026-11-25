#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "sdkconfig.h"

// -----------------------------------------------------------------------------

// Configures and controls a single onboard status LED.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes the configured onboard LED. Call once before setting its state.
void onboardLedInit();

// Sets the onboard LED output state.
void onboardLedSet(bool on);

#if CONFIG_BOARD_LED_TYPE_WS2812
// Sets the color of the configured WS2812 LED.
void onboardLedSetRGB(uint8_t red, uint8_t green, uint8_t blue);
#endif // CONFIG_BOARD_LED_TYPE_WS2812

#ifdef __cplusplus
}
#endif // __cplusplus
