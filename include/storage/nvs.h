#pragma once

#include "istorage.h"
#include <nvs.h>

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

// Non-volatile storage implementation of the IStorage interface using
// ESP-IDF NVS.
class Storage final : public IStorage
{
public:
    Storage(const char *nameSpace = nullptr, const char *partition = nullptr);
    ~Storage();

    esp_err_t readStr(const char *key, lightstd::string &str);
    esp_err_t writeStr(const char *key, const char *value);

    esp_err_t readBlob(const char *key, StorageBlob_t &blob);
    esp_err_t writeBlob(const char *key, const void *value, size_t valueLen);

    esp_err_t readInt(const char *key, int32_t *pValue);
    esp_err_t writeInt(const char *key, int32_t value);

    esp_err_t erase(const char *key);
    void eraseAll();

private:
    esp_err_t init();
    esp_err_t open(bool readOnly);

private:
    const char *nameSpace{nullptr};
    const char *partition{nullptr};
    nvs_handle_t handle{0};
    bool readOnlyMode{false};
};
