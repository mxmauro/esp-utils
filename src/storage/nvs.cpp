#include "storage/nvs.h"
#include "mutex.h"
#include <nvs_flash.h>

// -----------------------------------------------------------------------------

static esp_err_t convertError(esp_err_t err);

// -----------------------------------------------------------------------------

NVSStorage::NVSStorage(const char *_nameSpace, const char *_partition) noexcept
{
    nameSpace = _nameSpace ? _nameSpace : "storage";
    partition = _partition ? _partition : NVS_DEFAULT_PART_NAME;
}

NVSStorage::~NVSStorage() noexcept
{
    if (handle != 0) {
        nvs_close(handle);
        handle = 0;
    }
}

esp_err_t NVSStorage::readStr(const char *key, lightstd::string &str) noexcept
{
    size_t requiredSize;
    esp_err_t err;

    assert(key);
    assert(*key);
    str.clear();

    // Open storage (R/O access)
    err = open(true);
    if (err != ESP_OK) {
        goto exit;
    }

    // Get string size (includes nul terminator)
    requiredSize = 0;
    err = nvs_get_str(handle, key, nullptr, &requiredSize);
    if (err != ESP_OK) {
        goto exit;
    }

    // Allocate output
    if (!str.reserve(requiredSize)) {
        err = ESP_ERR_NO_MEM;
        goto exit;
    }

    // Get string
    err = nvs_get_str(handle, key, str.data(), &requiredSize);
    if (err != ESP_OK) {
        goto exit;
    }

    // Remove extra trailing \0
    if (requiredSize > 0) {
        requiredSize--;
    }
    if (!str.resize(requiredSize)) {
        err = ESP_ERR_NO_MEM;
        goto exit;
    }

    // Done
    err = ESP_OK;

exit:
    err = convertError(err);
    if (err != ESP_OK) {
        str.clear();
    }
    return err;
}

esp_err_t NVSStorage::writeStr(const char *key, const char *value) noexcept
{
    esp_err_t err;

    assert(key);
    assert(*key);

    // Open storage (R/W access)
    err = open(false);
    if (err != ESP_OK) {
        goto exit;
    }

    // Write string
    err = nvs_set_str(handle, key, value);

    // Cleanup
exit:
    return convertError(err);
}

esp_err_t NVSStorage::readBlob(const char *key, lightstd::vector<uint8_t> &blob) noexcept
{
    size_t requiredSize;
    esp_err_t err;

    assert(key);
    assert(*key);
    blob.clear();

    // Open storage (R/O access)
    err = open(true);
    if (err != ESP_OK) {
        goto exit;
    }

    // Get blob size
    requiredSize = 0;
    err = nvs_get_blob(handle, key, nullptr, &requiredSize);
    if (err != ESP_OK || requiredSize == 0) {
        goto exit;
    }

    // Allocate output
    if (!blob.resize(requiredSize)) {
        err = ESP_ERR_NO_MEM;
        goto exit;
    }

    // Get blob
    err = nvs_get_blob(handle, key, blob.data(), &requiredSize);
    if (err != ESP_OK) {
        goto exit;
    }

    // Done
    err = ESP_OK;

exit:
    err = convertError(err);
    if (err != ESP_OK) {
        blob.clear();
    }
    return err;
}

esp_err_t NVSStorage::writeBlob(const char *key, const void *value, size_t valueLen) noexcept
{
    esp_err_t err;

    assert(key);
    assert(*key);

    // Open storage (R/W access)
    err = open(false);
    if (err != ESP_OK) {
        goto exit;
    }

    // Write string
    err = nvs_set_blob(handle, key, value, valueLen);

    // Cleanup
exit:
    return convertError(err);
}

esp_err_t NVSStorage::readInt(const char *key, int32_t *pValue) noexcept
{
    esp_err_t err;

    assert(key);
    assert(*key);
    assert(pValue);
    *pValue = 0;

    // Open storage (R/O access)
    err = open(true);
    if (err != ESP_OK) {
        goto exit;
    }

    // Get int32
    err = nvs_get_i32(handle, key, pValue);

    // Cleanup
exit:
    return convertError(err);
}

esp_err_t NVSStorage::writeInt(const char *key, int32_t value) noexcept
{
    esp_err_t err;

    assert(key);
    assert(*key);

    // Open storage (R/W access)
    err = open(false);
    if (err != ESP_OK) {
        goto exit;
    }

    // Write int32
    err = nvs_set_i32(handle, key, value);

    // Cleanup
exit:
    return convertError(err);
}

esp_err_t NVSStorage::erase(const char *key) noexcept
{
    esp_err_t err;

    assert(key);
    assert(*key);

    // Open storage (R/W access)
    err = open(false);
    if (err != ESP_OK) {
        goto exit;
    }

    // Delete entry and ignore error if the entry doesn't exist.
    err = nvs_erase_key(handle, key);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND && err != ESP_ERR_NVS_NOT_FOUND) {
        goto exit;
    }

    // Done
    err = ESP_OK;

    // Cleanup
exit:
    return convertError(err);
}

void NVSStorage::eraseAll() noexcept
{
    nvs_flash_erase_partition(partition);
    nvs_flash_init_partition(partition);
}

esp_err_t NVSStorage::open(bool readOnly) noexcept
{
    esp_err_t err;

    if (handle != 0 && readOnlyMode && (!readOnly)) {
        nvs_close(handle);
        handle = 0;
    }

    if (handle != 0) {
        return ESP_OK;
    }

    err = init();
    if (err == ESP_OK) {
        err = nvs_open_from_partition(partition, nameSpace, readOnly ? NVS_READONLY : NVS_READWRITE, &handle);
        if (err == ESP_OK) {
            readOnlyMode = readOnly;
        }
        else {
            handle = 0;
        }
    }
    return convertError(err);
}

esp_err_t NVSStorage::init() noexcept
{
    static Mutex initMtx;
    static bool volatile initialized = false;

    if (!initialized) {
        AutoMutex lock(initMtx);

        if (!initialized) {
            esp_err_t err;

            err = nvs_flash_init();
            if ((err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) &&
                strcasecmp(partition, NVS_DEFAULT_PART_NAME) == 0
            ) {
                nvs_flash_erase();
                err = nvs_flash_init();
            }
            if (err != ESP_OK) {
                return err;
            }

            initialized = true;
        }
    }

    return nvs_flash_init_partition(partition);
}

// -----------------------------------------------------------------------------

static esp_err_t convertError(esp_err_t err)
{
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    }
    return err;
}
