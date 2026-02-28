#pragma once

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include "istorage.h"
#include <nvs.h>

// -----------------------------------------------------------------------------

// Non-volatile storage implementation of the IStorage interface using
// ESP-IDF NVS.
class NVSStorage final : public IStorage
{
public:
    NVSStorage(const char *nameSpace = nullptr, const char *partition = nullptr) noexcept;
    NVSStorage(const NVSStorage&) = delete;
    NVSStorage(NVSStorage&&) = delete;
    ~NVSStorage() noexcept;

    NVSStorage& operator=(const NVSStorage&) = delete;
    NVSStorage& operator=(NVSStorage&&) = delete;

    esp_err_t readStr(const char *key, lightstd::string &str) noexcept;
    esp_err_t writeStr(const char *key, const char *value) noexcept;

    esp_err_t readBlob(const char *key, lightstd::vector<uint8_t> &blob) noexcept;
    esp_err_t writeBlob(const char *key, const void *value, size_t valueLen) noexcept;

    esp_err_t readInt(const char *key, int32_t *pValue) noexcept;
    esp_err_t writeInt(const char *key, int32_t value) noexcept;

    esp_err_t erase(const char *key) noexcept;
    void eraseAll() noexcept;

private:
    esp_err_t init() noexcept;
    esp_err_t open(bool readOnly) noexcept;

private:
    const char *nameSpace{nullptr};
    const char *partition{nullptr};
    nvs_handle_t handle{0};
    bool readOnlyMode{false};
};
