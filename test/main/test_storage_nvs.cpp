#include <unity.h>
#include "storage/nvs.h"

// -----------------------------------------------------------------------------

TEST_CASE("NVSStorage write and read string", "storage nvs")
{
    NVSStorage       storage("ut_nvs");
    lightstd::string value;

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeStr("name", "esp-utils"));
    TEST_ASSERT_EQUAL(ESP_OK, storage.commit());
    TEST_ASSERT_EQUAL(ESP_OK, storage.readStr("name", value));
    TEST_ASSERT_EQUAL_STRING("esp-utils", value.c_str());
}

TEST_CASE("NVSStorage write and read int", "storage nvs")
{
    NVSStorage storage("ut_nvs");
    int32_t    value = 0;

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeInt("counter", 12345));
    TEST_ASSERT_EQUAL(ESP_OK, storage.commit());
    TEST_ASSERT_EQUAL(ESP_OK, storage.readInt("counter", &value));
    TEST_ASSERT_EQUAL_INT32(12345, value);
}

TEST_CASE("NVSStorage write and read blob", "storage nvs")
{
    NVSStorage                storage("ut_nvs");
    lightstd::vector<uint8_t> blob;
    const uint8_t             expected[] = {0x01, 0x10, 0x7F, 0xFF};

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeBlob("blob", expected, sizeof(expected)));
    TEST_ASSERT_EQUAL(ESP_OK, storage.commit());
    TEST_ASSERT_EQUAL(ESP_OK, storage.readBlob("blob", blob));

    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), blob.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, blob.data(), sizeof(expected));
}

TEST_CASE("NVSStorage erase and missing key", "storage nvs")
{
    NVSStorage       storage("ut_nvs");
    lightstd::string value;

    TEST_ASSERT_EQUAL(ESP_OK, storage.writeStr("temp", "x"));
    TEST_ASSERT_EQUAL(ESP_OK, storage.commit());
    TEST_ASSERT_EQUAL(ESP_OK, storage.erase("temp"));
    TEST_ASSERT_EQUAL(ESP_OK, storage.commit());

    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, storage.readStr("temp", value));
    TEST_ASSERT_EQUAL_STRING("", value.c_str());

    TEST_ASSERT_EQUAL(ESP_OK, storage.erase("temp"));
    TEST_ASSERT_EQUAL(ESP_OK, storage.commit());
}

TEST_CASE("NVSStorage persists committed values across instances", "storage nvs lifecycle")
{
    {
        NVSStorage writer("ut_nvs_persist");

        TEST_ASSERT_EQUAL(ESP_OK, writer.writeInt("counter", 2468));
        TEST_ASSERT_EQUAL(ESP_OK, writer.commit());
    }

    NVSStorage reader("ut_nvs_persist");
    int32_t    value = 0;

    TEST_ASSERT_EQUAL(ESP_OK, reader.readInt("counter", &value));
    TEST_ASSERT_EQUAL_INT32(2468, value);
}

TEST_CASE("NVSStorage promotes a read-only handle for writes", "storage nvs lifecycle")
{
    NVSStorage storage("ut_nvs_promote");
    int32_t    value = 123;

    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, storage.readInt("missing", &value));
    TEST_ASSERT_EQUAL_INT32(0, value);
    TEST_ASSERT_EQUAL(ESP_OK, storage.writeInt("counter", 99));
    TEST_ASSERT_EQUAL(ESP_OK, storage.commit());
    TEST_ASSERT_EQUAL(ESP_OK, storage.readInt("counter", &value));
    TEST_ASSERT_EQUAL_INT32(99, value);
}

TEST_CASE("NVSStorage clears outputs for missing blob values", "storage nvs failure paths")
{
    NVSStorage                storage("ut_nvs_missing");
    lightstd::vector<uint8_t> blob;

    TEST_ASSERT_TRUE(blob.push_back(1));
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, storage.readBlob("missing", blob));
    TEST_ASSERT_TRUE(blob.empty());
}
