#pragma once

#include <stdint.h>

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Returns the uptime in milliseconds since system boot.
uint64_t now_ms(void);

#ifdef __cplusplus
}
#endif // __cplusplus
