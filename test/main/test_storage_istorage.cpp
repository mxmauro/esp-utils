#include <unity.h>

#include "storage/istorage.h"
#include "storage/nvs.h"

// -----------------------------------------------------------------------------

TEST_CASE("IStorage interface works with NVSStorage", "storage istorage")
{
    NVSStorage nvs("ut_nvs_iface");
    IStorage *storage = &nvs;

    TEST_ASSERT_EQUAL(ESP_OK, storage->writeInt("iface_counter", 77));

    int32_t value = 0;
    TEST_ASSERT_EQUAL(ESP_OK, storage->readInt("iface_counter", &value));
    TEST_ASSERT_EQUAL_INT32(77, value);

    TEST_ASSERT_EQUAL(ESP_OK, storage->erase("iface_counter"));
}
