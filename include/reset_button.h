#pragma once

#include <driver/gpio.h>

// -----------------------------------------------------------------------------

typedef void (*ResetButtonPressedHandler_t)(void *ctx);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Configures a GPIO as a reset button and registers its press handler.
void setupResetButton(gpio_num_t gpioPin, ResetButtonPressedHandler_t handler, void *ctx = nullptr);

#ifdef __cplusplus
}
#endif // __cplusplus
