#pragma once

#include <stdint.h>

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Returns the current time in milliseconds since system booted.
uint64_t now_ms(void);

#ifdef __cplusplus
}
#endif // __cplusplus
