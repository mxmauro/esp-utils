#include "convert.h"
#include <esp_err.h>

// -----------------------------------------------------------------------------

static const char* hexaChars = "0123456789ABCDEF";

// -----------------------------------------------------------------------------

static char        b64EncodeChar(uint8_t v, bool isUrl);
static int8_t      b64DecodeChar(char c, bool isUrl);
static bool        b64DecodedSize(const char* src, size_t srcLen, bool isUrl, size_t* decodedSize);
static inline bool isBlank(char c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

// -----------------------------------------------------------------------------

bool toHex(const void* src, size_t srcLen, char* dest, size_t* destLen)
{
    size_t requiredSize;
    size_t origDestLen;

    if (!destLen)
    {
        return false;
    }
    if (srcLen > SIZE_MAX / 2)
    {
        *destLen = SIZE_MAX;
        return false;
    }

    requiredSize = srcLen * 2;
    origDestLen = *destLen;
    if (origDestLen < requiredSize || (!dest && origDestLen > 0))
    {
        *destLen = requiredSize;
        return false;
    }
    if (srcLen > 0 && !src)
    {
        *destLen = 0;
        return false;
    }

    *destLen = srcLen * 2;

    const uint8_t* ptr = static_cast<const uint8_t*>(src);
    while (srcLen > 0)
    {
        *dest++ = hexaChars[(*ptr) >> 4];
        *dest++ = hexaChars[(*ptr) & 0x0F];

        ptr += 1;
        srcLen -= 1;
    }

    if (origDestLen > *destLen)
    {
        *dest = 0;
    }
    return true;
}

bool fromHex(const char* src, size_t srcLen, uint8_t* dest, size_t* destLen)
{
    size_t requiredSize;

    if (!destLen)
    {
        return false;
    }

    requiredSize = srcLen / 2;
    if ((srcLen & 1) != 0 || *destLen < requiredSize || (requiredSize > 0 && !dest))
    {
        *destLen = requiredSize;
        return false;
    }
    if (srcLen > 0 && !src)
    {
        *destLen = 0;
        return false;
    }

    *destLen = requiredSize;

    while (srcLen > 0)
    {
        uint8_t v;

        if (*src >= '0' && *src <= '9')
        {
            v = static_cast<uint8_t>(*src - '0');
        }
        else if (*src >= 'A' && *src <= 'F')
        {
            v = static_cast<uint8_t>(*src - 'A') + 10;
        }
        else if (*src >= 'a' && *src <= 'f')
        {
            v = static_cast<uint8_t>(*src - 'a') + 10;
        }
        else
        {
            *destLen = 0;
            return false;
        }

        if ((srcLen & 1) == 0)
        {
            *dest = v << 4;
        }
        else
        {
            *dest |= v;
            dest++;
        }

        src += 1;
        srcLen -= 1;
    }

    return true;
}

bool toB64(const void* src, size_t srcLen, bool isUrl, char* dest, size_t* destLen)
{
    size_t   full = srcLen / 3;
    size_t   rem = srcLen % 3;
    size_t   requiredDestLen;
    uint32_t v;

    if (full > SIZE_MAX / 4)
    {
        if (destLen)
        {
            *destLen = SIZE_MAX;
        }
        return false;
    }

    if (!isUrl)
    {
        // Standard base64 uses '=' padding
        requiredDestLen = full * 4;
        if (rem)
        {
            if (requiredDestLen > SIZE_MAX - 4)
            {
                if (destLen)
                {
                    *destLen = SIZE_MAX;
                }
                return false;
            }
            requiredDestLen += 4;
        }
    }
    else
    {
        // URL-safe form without padding
        // 0 -> +0, 1 -> +2 chars, 2 -> +3 chars
        requiredDestLen = 4 * full;
        if (rem)
        {
            if (requiredDestLen > SIZE_MAX - (rem + 1))
            {
                if (destLen)
                {
                    *destLen = SIZE_MAX;
                }
                return false;
            }
            requiredDestLen += rem + 1;
        }
    }

    if (!destLen)
    {
        return false;
    }
    if (*destLen < requiredDestLen || (!dest && *destLen > 0))
    {
        *destLen = requiredDestLen;
        return false;
    }
    if (srcLen > 0 && !src)
    {
        *destLen = 0;
        return false;
    }

    const uint8_t* s = static_cast<const uint8_t*>(src);
    size_t         out = 0;

    for (size_t i = 0; i < full; ++i)
    {
        v = (static_cast<uint32_t>(s[3 * i]) << 16) | (static_cast<uint32_t>(s[3 * i + 1]) << 8) | static_cast<uint32_t>(s[3 * i + 2]);
        dest[out++] = b64EncodeChar((v >> 18) & 0x3F, isUrl);
        dest[out++] = b64EncodeChar((v >> 12) & 0x3F, isUrl);
        dest[out++] = b64EncodeChar((v >> 6) & 0x3F, isUrl);
        dest[out++] = b64EncodeChar(v & 0x3F, isUrl);
    }

    switch (rem)
    {
        case 1:
            v = static_cast<uint32_t>(s[3 * full]) << 16;
            dest[out++] = b64EncodeChar((v >> 18) & 0x3F, isUrl);
            dest[out++] = b64EncodeChar((v >> 12) & 0x3F, isUrl);
            if (!isUrl)
            {
                dest[out++] = '=';
                dest[out++] = '=';
            }
            break;

        case 2:
            v = (static_cast<uint32_t>(s[3 * full]) << 16) | (static_cast<uint32_t>(s[3 * full + 1]) << 8);
            dest[out++] = b64EncodeChar((v >> 18) & 0x3F, isUrl);
            dest[out++] = b64EncodeChar((v >> 12) & 0x3F, isUrl);
            dest[out++] = b64EncodeChar((v >> 6) & 0x3F, isUrl);
            if (!isUrl)
            {
                dest[out++] = '=';
            }
            break;
    }

    if (out < *destLen)
    {
        dest[out] = 0;
    }
    *destLen = out;

    return true;
}

bool fromB64(const char* src, size_t srcLen, bool isUrl, uint8_t* dest, size_t* destLen)
{
    uint8_t vbuf[4];
    int     vCount = 0;
    size_t  out = 0;
    size_t  requiredSize;

    if (!destLen)
    {
        return false;
    }
    if (!b64DecodedSize(src, srcLen, isUrl, &requiredSize))
    {
        *destLen = 0;
        return false;
    }
    if (*destLen < requiredSize || (requiredSize > 0 && !dest))
    {
        *destLen = requiredSize;
        return false;
    }

    for (size_t i = 0; i < srcLen; i++)
    {
        char c = src[i];

        if (isBlank(c))
        {
            continue;
        }

        if (c == '=')
        {
            break;
        }

        vbuf[vCount++] = static_cast<uint8_t>(b64DecodeChar(c, isUrl));

        if (vCount == 4)
        {
            // Produce 3 bytes
            dest[out++] = (vbuf[0] << 2) | (vbuf[1] >> 4);
            dest[out++] = (vbuf[1] << 4) | (vbuf[2] >> 2);
            dest[out++] = (vbuf[2] << 6) | vbuf[3];
            vCount = 0;
        }
    }

    // Finalize
    switch (vCount)
    {
        case 2:
            dest[out++] = (vbuf[0] << 2) | (vbuf[1] >> 4);
            break;

        case 3:
            dest[out++] = (vbuf[0] << 2) | (vbuf[1] >> 4);
            dest[out++] = (vbuf[1] << 4) | (vbuf[2] >> 2);
            break;
    }

    // Done
    *destLen = requiredSize;
    return true;
}

// -----------------------------------------------------------------------------

static char b64EncodeChar(uint8_t v, bool isUrl)
{
    // v in [0..63]
    if (v < 26)
    {
        return static_cast<char>('A' + v);
    }
    if (v < 52)
    {
        return static_cast<char>('a' + (v - 26));
    }
    if (v < 62)
    {
        return static_cast<char>('0' + (v - 52));
    }
    return (v == 62) ? (isUrl ? '-' : '+') : (isUrl ? '_' : '/');
}

static int8_t b64DecodeChar(char c, bool isUrl)
{
    if (c >= 'A' && c <= 'Z')
    {
        return static_cast<int8_t>(c - 'A');
    }
    if (c >= 'a' && c <= 'z')
    {
        return static_cast<int8_t>(26 + (c - 'a'));
    }
    if (c >= '0' && c <= '9')
    {
        return static_cast<int8_t>(52 + (c - '0'));
    }
    if (!isUrl)
    {
        if (c == '+')
        {
            return 62;
        }
        if (c == '/')
        {
            return 63;
        }
    }
    else
    {
        if (c == '-')
        {
            return 62;
        }
        if (c == '_')
        {
            return 63;
        }
    }
    return -1; // not an alphabet char
}

static bool b64DecodedSize(const char* src, size_t srcLen, bool isUrl, size_t* decodedSize)
{
    size_t symbols = 0;
    size_t padding = 0;
    bool   seenPadding = false;

    if (!src && srcLen > 0)
    {
        return false;
    }

    for (size_t i = 0; i < srcLen; i++)
    {
        char c = src[i];

        if (isBlank(c))
        {
            continue;
        }
        if (c == '=')
        {
            seenPadding = true;
            padding++;
            continue;
        }
        if (seenPadding || b64DecodeChar(c, isUrl) < 0)
        {
            return false;
        }
        if (symbols == SIZE_MAX)
        {
            return false;
        }
        symbols++;
    }

    if (isUrl)
    {
        if (padding > 0 || symbols % 4 == 1)
        {
            return false;
        }
    }
    else if (padding > 0)
    {
        if (padding > 2 || symbols % 4 + padding != 4)
        {
            return false;
        }
    }
    else if (symbols % 4 != 0)
    {
        return false;
    }

    size_t fullGroups = symbols / 4;
    size_t remainder = symbols % 4;
    if (fullGroups > SIZE_MAX / 3)
    {
        return false;
    }

    size_t trailingBytes = remainder ? remainder - 1 : 0;
    size_t fullBytes = fullGroups * 3;
    if (fullBytes > SIZE_MAX - trailingBytes)
    {
        return false;
    }

    *decodedSize = fullBytes + trailingBytes;
    return true;
}
