#include <string.h>
#include <unity.h>
#include "convert.h"

// -----------------------------------------------------------------------------

TEST_CASE("Convert hex roundtrip", "toHex/fromHex success path")
{
    const uint8_t input[] = {0xDE, 0xAD, 0xBE, 0xEF};
    char          hex[32];
    size_t        hexLen = sizeof(hex);

    TEST_ASSERT_TRUE(toHex(input, sizeof(input), hex, &hexLen));
    TEST_ASSERT_EQUAL_UINT32(8, hexLen);
    TEST_ASSERT_EQUAL_STRING("DEADBEEF", hex);

    uint8_t output[8] = {};
    size_t  outputLen = sizeof(output);
    TEST_ASSERT_TRUE(fromHex(hex, hexLen, output, &outputLen));
    TEST_ASSERT_EQUAL_UINT32(sizeof(input), outputLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, output, sizeof(input));
}

TEST_CASE("Convert hex reports required size", "toHex failure path")
{
    const uint8_t input[] = {0x01, 0x23, 0x45};
    char          hex[4];
    size_t        hexLen = sizeof(hex);

    TEST_ASSERT_FALSE(toHex(input, sizeof(input), hex, &hexLen));
    TEST_ASSERT_EQUAL_UINT32(HEX_ENCODE_SIZE(sizeof(input)), hexLen);
}

TEST_CASE("Convert hex rejects invalid input", "fromHex failure paths")
{
    uint8_t output[8] = {};
    size_t  outputLen;

    outputLen = sizeof(output);
    TEST_ASSERT_FALSE(fromHex("ABC", 3, output, &outputLen));
    TEST_ASSERT_EQUAL_UINT32(1, outputLen);

    outputLen = sizeof(output);
    TEST_ASSERT_FALSE(fromHex("GG", 2, output, &outputLen));
    TEST_ASSERT_EQUAL_UINT32(0, outputLen);
}

TEST_CASE("Convert b64 standard roundtrip", "toB64/fromB64 with padding")
{
    static const char* kText = "foobar";
    char               encoded[32];
    size_t             encodedLen = sizeof(encoded);
    uint8_t            decoded[16] = {};
    size_t             decodedLen = sizeof(decoded);

    TEST_ASSERT_TRUE(toB64(kText, strlen(kText), false, encoded, &encodedLen));
    TEST_ASSERT_EQUAL_STRING("Zm9vYmFy", encoded);
    TEST_ASSERT_EQUAL_UINT32(8, encodedLen);

    TEST_ASSERT_TRUE(fromB64(encoded, encodedLen, false, decoded, &decodedLen));
    TEST_ASSERT_EQUAL_UINT32(strlen(kText), decodedLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t*>(kText), decoded, decodedLen);
}

TEST_CASE("Convert b64 URL-safe encoding", "toB64 no padding for URL-safe mode")
{
    const uint8_t input[] = {0xF8, 0x00, 0x00};
    char          encoded[16];
    size_t        encodedLen = sizeof(encoded);

    TEST_ASSERT_TRUE(toB64(input, sizeof(input), true, encoded, &encodedLen));
    TEST_ASSERT_EQUAL_STRING("-AAA", encoded);
    TEST_ASSERT_EQUAL_UINT32(4, encodedLen);
}

TEST_CASE("Convert b64 decoder handles whitespace", "fromB64 accepts blank separators")
{
    static const char* kEncoded = " Zm9v \n YmFy\t";
    uint8_t            decoded[16] = {};
    size_t             decodedLen = sizeof(decoded);
    static const char* kExpected = "foobar";

    TEST_ASSERT_TRUE(fromB64(kEncoded, strlen(kEncoded), false, decoded, &decodedLen));
    TEST_ASSERT_EQUAL_UINT32(strlen(kExpected), decodedLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t*>(kExpected), decoded, decodedLen);
}

TEST_CASE("Convert b64 decoder rejects invalid chars", "fromB64 failure path")
{
    uint8_t decoded[8] = {};
    size_t  decodedLen = sizeof(decoded);

    TEST_ASSERT_FALSE(fromB64("Zm$=", 4, false, decoded, &decodedLen));
}

TEST_CASE("Convert b64 reports exact required decode size", "fromB64 capacity query")
{
    size_t decodedLen = 0;

    TEST_ASSERT_FALSE(fromB64("TQ==", 4, false, nullptr, &decodedLen));
    TEST_ASSERT_EQUAL_UINT32(1, decodedLen);
}

TEST_CASE("Convert b64 reports required encode capacity", "toB64 capacity query")
{
    char   encoded[4];
    size_t encodedLen = 3;

    TEST_ASSERT_FALSE(toB64("M", 1, false, encoded, &encodedLen));
    TEST_ASSERT_EQUAL_UINT32(4, encodedLen);

    encodedLen = sizeof(encoded);
    TEST_ASSERT_TRUE(toB64("M", 1, false, encoded, &encodedLen));
    TEST_ASSERT_EQUAL_UINT32(4, encodedLen);
    TEST_ASSERT_EQUAL_MEMORY("TQ==", encoded, sizeof(encoded));
}

TEST_CASE("Convert b64 URL-safe decoder accepts unpadded input", "fromB64 URL-safe decoding")
{
    uint8_t decoded[2] = {};
    size_t  decodedLen = sizeof(decoded);

    TEST_ASSERT_TRUE(fromB64("TQ", 2, true, decoded, &decodedLen));
    TEST_ASSERT_EQUAL_UINT32(1, decodedLen);
    TEST_ASSERT_EQUAL_UINT8('M', decoded[0]);
}

TEST_CASE("Convert b64 rejects malformed padding", "fromB64 failure path")
{
    uint8_t decoded[4] = {};
    size_t  decodedLen = sizeof(decoded);

    TEST_ASSERT_FALSE(fromB64("T=Q=", 4, false, decoded, &decodedLen));
    decodedLen = sizeof(decoded);
    TEST_ASSERT_FALSE(fromB64("TQ===", 5, false, decoded, &decodedLen));
    decodedLen = sizeof(decoded);
    TEST_ASSERT_FALSE(fromB64("TQ", 2, false, decoded, &decodedLen));
}

TEST_CASE("Convert rejects overflowing encode sizes", "conversion bounds")
{
    char   output[1];
    size_t outputLen = sizeof(output);

    TEST_ASSERT_FALSE(toHex(nullptr, SIZE_MAX, output, &outputLen));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, outputLen);

    outputLen = sizeof(output);
    TEST_ASSERT_FALSE(toB64(nullptr, SIZE_MAX, false, output, &outputLen));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, outputLen);
}
