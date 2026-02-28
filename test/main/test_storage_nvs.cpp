#include <unity.h>

#include "storage/nvs.h"

// -----------------------------------------------------------------------------

TEST_CASE("NVSStorage write and read string", "storage nvs")
{
    NVSStorage storage("ut_nvs");
    lightstd::string value;

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeStr("name", "esp-utils"));
    TEST_ASSERT_EQUAL(ESP_OK, storage.readStr("name", value));
    TEST_ASSERT_EQUAL_STRING("esp-utils", value.c_str());
}

TEST_CASE("NVSStorage write and read int", "storage nvs")
{
    NVSStorage storage("ut_nvs");
    int32_t value = 0;

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeInt("counter", 12345));
    TEST_ASSERT_EQUAL(ESP_OK, storage.readInt("counter", &value));
    TEST_ASSERT_EQUAL_INT32(12345, value);
}

TEST_CASE("NVSStorage write and read blob", "storage nvs")
{
    NVSStorage storage("ut_nvs");
    lightstd::vector<uint8_t> blob;
    const uint8_t expected[] = {0x01, 0x10, 0x7F, 0xFF};

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeBlob("blob", expected, sizeof(expected)));
    TEST_ASSERT_EQUAL(ESP_OK, storage.readBlob("blob", blob));

    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), blob.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, blob.data(), sizeof(expected));
}

TEST_CASE("NVSStorage erase and missing key", "storage nvs")
{
    NVSStorage storage("ut_nvs");
    lightstd::string value;

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeStr("temp", "x"));
    TEST_ASSERT_EQUAL(ESP_OK, storage.erase("temp"));

    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, storage.readStr("temp", value));
    TEST_ASSERT_EQUAL_STRING("", value.c_str());

    TEST_ASSERT_EQUAL(ESP_OK, storage.erase("temp"));
}
