#pragma once

#include <stdlib.h>

#define HEX_ENCODE_SIZE(srcLen) (srcLen * 2)
#define B64_ENCODE_SIZE(srcLen) (4 * ((srcLen + 2) / 3))

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Encodes raw bytes as lowercase hexadecimal text.
// destLen, on input, contains the destination buffer size and, on output,
// the number of written characters excluding the added trailing nul.
// IMPORTANT: A trailing nul is written only if enough space is available in the output buffer.
bool toHex(const void *src, size_t srcLen, char *dest, size_t *destLen);

// Decodes hexadecimal text into raw bytes.
// destLen, on input, contains the destination buffer size and, on output,
// the number of written bytes.
bool fromHex(const char *src, size_t srcLen, uint8_t *dest, size_t *destLen);

// Encodes raw bytes as standard or URL-safe Base64 text.
// destLen, on input, contains the destination buffer size and, on output,
// the number of written characters excluding the added trailing nul.
// IMPORTANT: A trailing nul is written only if enough space is available in the output buffer.
bool toB64(const void *src, size_t srcLen, bool isUrl, char *dest, size_t *destLen);

// Decodes standard or URL-safe Base64 text into raw bytes.
// destLen, on input, contains the destination buffer size and, on output,
// the number of written bytes.
bool fromB64(const char *src, size_t srcLen, bool isUrl, uint8_t *dest, size_t *destLen);

#ifdef __cplusplus
}
#endif // __cplusplus
