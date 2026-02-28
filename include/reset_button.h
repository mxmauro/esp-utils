#pragma once

#include <driver/gpio.h>

// -----------------------------------------------------------------------------

typedef void (*ResetButtonPressedHandler_t)(void *ctx);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Setup the reset button on the specified GPIO pin and call the handler
// when pressed.
void setupResetButton(gpio_num_t gpioPin, ResetButtonPressedHandler_t handler, void *ctx = nullptr);

#ifdef __cplusplus
}
#endif // __cplusplus
