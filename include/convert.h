#pragma once

#include <stdlib.h>

#define HEX_ENCODE_SIZE(srcLen) (srcLen * 2)
#define B64_ENCODE_SIZE(srcLen) (4 * ((srcLen + 2) / 3))

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// destLen, on input, contains the destination buffer size and, on output,
// the number of written characters excluding the added trailing nul.
// IMPORTANT: Traliling nul will be written only if enough space in the
//            output bufer.
bool toHex(const void *src, size_t srcLen, char *dest, size_t *destLen);

// destLen, on input, contains the destination buffer size and, on output,
// the number of written bytes.
bool fromHex(const char *src, size_t srcLen, uint8_t *dest, size_t *destLen);

// destLen, on input, contains the destination buffer size and, on output,
// the number of written characters excluding the added trailing nul.
// IMPORTANT: Traliling nul will be written only if enough space in the
//            output bufer.
bool toB64(const void *src, size_t srcLen, bool isUrl, char *dest, size_t *destLen);

// destLen, on input, contains the destination buffer size and, on output,
// the number of written bytes.
bool fromB64(const char *src, size_t srcLen, bool isUrl, uint8_t *dest, size_t *destLen);

#ifdef __cplusplus
}
#endif // __cplusplus
