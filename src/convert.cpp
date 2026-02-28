#include "convert.h"
#include <esp_err.h>

// -----------------------------------------------------------------------------

static const char *hexaChars = "0123456789ABCDEF";

// -----------------------------------------------------------------------------

static char b64EncodeChar(uint8_t v, bool isUrl);
static int8_t b64DecodeChar(char c, bool isUrl);
static inline bool isBlank(char c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

// -----------------------------------------------------------------------------

bool toHex(const void *src, size_t srcLen, char *dest, size_t *destLen)
{
    size_t origDestLen = *destLen;
    if (origDestLen < HEX_ENCODE_SIZE(srcLen)) {
        *destLen = HEX_ENCODE_SIZE(srcLen);
        return false;
    }

    *destLen = srcLen * 2;

    const uint8_t *ptr = (const uint8_t *)src;
    while (srcLen > 0) {
        *dest++ = hexaChars[(*ptr) >> 4];
        *dest++ = hexaChars[(*ptr) & 0x0F];

        ptr += 1;
        srcLen -= 1;
    }

    if (origDestLen > *destLen) {
        *dest = 0;
    }
    return true;
}

bool fromHex(const char *src, size_t srcLen, uint8_t *dest, size_t *destLen)
{
    if ((srcLen & 1) != 0 || *destLen < srcLen / 2) {
        *destLen = srcLen / 2;
        return false;
    }

    *destLen = srcLen / 2;

    while (srcLen > 0) {
        uint8_t v;

        if (*src >= '0' && *src <= '9') {
            v = (uint8_t)(*src - '0');
        }
        else if (*src >= 'A' && *src <= 'F') {
            v = (uint8_t)(*src - 'A') + 10;
        }
        else if (*src >= 'a' && *src <= 'f') {
            v = (uint8_t)(*src - 'a') + 10;
        }
        else {
            *destLen = 0;
            return false;
        }

        if ((srcLen & 1) == 0) {
            *dest = v << 4;
        }
        else {
            *dest |= v;
            dest++;
        }

        src += 1;
        srcLen -= 1;
    }

    return true;
}

bool toB64(const void *src, size_t srcLen, bool isUrl, char *dest, size_t *destLen)
{
    size_t full = srcLen / 3;
    size_t rem  = srcLen % 3;
    size_t requiredDestLen;
    uint32_t v;

    if (!isUrl) {
        // Standard base64 uses '=' padding
        requiredDestLen = B64_ENCODE_SIZE(srcLen);
    }
    else {
        // URL-safe form without padding
        // 0 -> +0, 1 -> +2 chars, 2 -> +3 chars
        requiredDestLen = 4 * full + (rem ? (rem + 1) : 0);
    }

    if (*destLen < requiredDestLen) {
        *destLen = requiredDestLen;
        return false;
    }

    const uint8_t *s = (const uint8_t *)src;
    size_t out = 0;

    for (size_t i = 0; i < full; ++i) {
        v = ((uint32_t)s[3 * i    ] << 16) |
            ((uint32_t)s[3 * i + 1] <<  8) |
             (uint32_t)s[3 * i + 2];
        dest[out++] = b64EncodeChar((v >> 18) & 0x3F, isUrl);
        dest[out++] = b64EncodeChar((v >> 12) & 0x3F, isUrl);
        dest[out++] = b64EncodeChar((v >>  6) & 0x3F, isUrl);
        dest[out++] = b64EncodeChar( v        & 0x3F, isUrl);
    }

    switch (rem) {
        case 1:
            v = ((uint32_t)s[3 * full]) << 16;
            dest[out++] = b64EncodeChar((v >> 18) & 0x3F, isUrl);
            dest[out++] = b64EncodeChar((v >> 12) & 0x3F, isUrl);
            if (!isUrl) {
                dest[out++] = '=';
                dest[out++] = '=';
            }
            break;

        case 2:
            v = (((uint32_t)s[3 * full    ]) << 16) |
                (((uint32_t)s[3 * full + 1]) <<  8);
            dest[out++] = b64EncodeChar((v >> 18) & 0x3F, isUrl);
            dest[out++] = b64EncodeChar((v >> 12) & 0x3F, isUrl);
            dest[out++] = b64EncodeChar((v >>  6) & 0x3F, isUrl);
            if (!isUrl) {
                dest[out++] = '=';
            }
            break;
    }

    if (out < *destLen) {
        dest[out] = 0;
    }
    *destLen = out;

    return true;
}

bool fromB64(const char *src, size_t srcLen, bool isUrl, uint8_t *dest, size_t *destLen)
{
    uint8_t vbuf[4];
    int vCount = 0;
    bool seenPad = false;
    size_t out = 0;
    int8_t v;

    size_t maxBufSize = *destLen;
    *destLen = (srcLen / 4) * 3; // Guess size based on input

    for (size_t i = 0; i < srcLen; i++) {
        char c = src[i];

        if (isBlank(c)) {
            continue;
        }

        if (c == '=') {
            int equalSignsCount = 1;

            seenPad = true;

            for (++i; i < srcLen; i++) {
                c = src[i];
                if (c == '=') {
                    equalSignsCount++;
                    if (equalSignsCount > 3) {
                        return false;
                    }
                }
                else if (!isBlank(c)) {
                    return false;
                }
            }
            if (equalSignsCount + vCount != 4) {
                return false;
            }

            // Quit main loop
            break;
        }

        v = b64DecodeChar(c, isUrl);
        if (v < 0) {
            return false;
        }
        vbuf[vCount++] = (uint8_t)v;

        if (vCount == 4) {
            // Produce 3 bytes
            if (out + 3 > maxBufSize) {
                return false;
            }
            dest[out++] = (vbuf[0] << 2) | (vbuf[1] >> 4);
            dest[out++] = (vbuf[1] << 4) | (vbuf[2] >> 2);
            dest[out++] = (vbuf[2] << 6) |  vbuf[3];
            vCount = 0;
        }
    }

    if (vCount == 1 || seenPad == (vCount == 0 || isUrl)) {
        return false;
    }

    // Finalize
    switch (vCount) {
        case 2:
            if (out + 1 > maxBufSize) {
                return false;
            }
            dest[out++] = (vbuf[0] << 2) | (vbuf[1] >> 4);
            break;

        case 3:
            if (out + 2 > maxBufSize) {
                return false;
            }
            dest[out++] = (vbuf[0] << 2) | (vbuf[1] >> 4);
            dest[out++] = (vbuf[1] << 4) | (vbuf[2] >> 2);
            break;
    }

    // Done
    *destLen = out;
    return true;
}

// -----------------------------------------------------------------------------

static char b64EncodeChar(uint8_t v, bool isUrl)
{
    // v in [0..63]
    if (v < 26) return (char)('A' + v);
    if (v < 52) return (char)('a' + (v - 26));
    if (v < 62) return (char)('0' + (v - 52));
    return (v == 62) ? (isUrl ? '-' : '+') : (isUrl ? '_' : '/');
}

static int8_t b64DecodeChar(char c, bool isUrl)
{
    if (c >= 'A' && c <= 'Z') {
        return (int8_t)(c - 'A');
    }
    if (c >= 'a' && c <= 'z') {
        return (int8_t)(26 + (c - 'a'));
    }
    if (c >= '0' && c <= '9') {
        return (int8_t)(52 + (c - '0'));
    }
    if (!isUrl) {
        if (c == '+') {
            return 62;
        }
        if (c == '/') {
            return 63;
        }
    }
    else {
        if (c == '-') {
            return 62;
        }
        if (c == '_') {
            return 63;
        }
    }
    return -1; // not an alphabet char
}
