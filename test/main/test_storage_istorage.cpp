#include <unity.h>
#include <type_traits>
#include "storage/istorage.h"
#include "storage/nvs.h"

// -----------------------------------------------------------------------------

static_assert(std::has_virtual_destructor_v<IStorage>);
static_assert(noexcept(static_cast<IStorage*>(nullptr)->readStr(nullptr, *static_cast<lightstd::string*>(nullptr))));
static_assert(noexcept(static_cast<IStorage*>(nullptr)->writeStr(nullptr, nullptr)));
static_assert(noexcept(static_cast<IStorage*>(nullptr)->readBlob(nullptr, *static_cast<lightstd::vector<uint8_t>*>(nullptr))));
static_assert(noexcept(static_cast<IStorage*>(nullptr)->writeBlob(nullptr, nullptr, 0)));
static_assert(noexcept(static_cast<IStorage*>(nullptr)->readInt(nullptr, nullptr)));
static_assert(noexcept(static_cast<IStorage*>(nullptr)->writeInt(nullptr, 0)));
static_assert(noexcept(static_cast<IStorage*>(nullptr)->erase(nullptr)));
static_assert(noexcept(static_cast<IStorage*>(nullptr)->commit()));

TEST_CASE("IStorage interface works with NVSStorage", "storage istorage")
{
    NVSStorage nvs("ut_nvs_iface");
    IStorage*  storage = &nvs;

    TEST_ASSERT_EQUAL(ESP_OK, storage->writeInt("iface_counter", 77));
    TEST_ASSERT_EQUAL(ESP_OK, storage->commit());

    int32_t value = 0;
    TEST_ASSERT_EQUAL(ESP_OK, storage->readInt("iface_counter", &value));
    TEST_ASSERT_EQUAL_INT32(77, value);

    TEST_ASSERT_EQUAL(ESP_OK, storage->erase("iface_counter"));
    TEST_ASSERT_EQUAL(ESP_OK, storage->commit());
}
