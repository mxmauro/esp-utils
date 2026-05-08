#pragma once

#ifdef __cplusplus
    #include "istorage.h"
#endif // __cplusplus
#include <esp_err.h>
#include <nvs.h>

// -----------------------------------------------------------------------------

// Initializes the shared NVS subsystem for this component.
esp_err_t nvsInit();

#ifdef __cplusplus

// Non-volatile storage implementation of the IStorage interface using
// ESP-IDF NVS.
class NVSStorage final : public IStorage
{
public:
    // Opens an NVS-backed storage view for the selected namespace and partition.
    NVSStorage(const char *nameSpace = nullptr, const char *partition = nullptr) noexcept;
    NVSStorage(const NVSStorage&) = delete;
    // Transfers ownership of the underlying NVS handle.
    NVSStorage(NVSStorage&& other) noexcept;
    // Closes the owned NVS handle if one is open.
    ~NVSStorage() noexcept;

    NVSStorage& operator=(const NVSStorage&) = delete;
    // Replaces this storage view with another storage handle owner.
    NVSStorage& operator=(NVSStorage&& other) noexcept;

    // Reads a string value from NVS.
    esp_err_t readStr(const char *key, lightstd::string &str) noexcept;
    // Writes a string value to NVS.
    esp_err_t writeStr(const char *key, const char *value) noexcept;

    // Reads a blob value from NVS.
    esp_err_t readBlob(const char *key, lightstd::vector<uint8_t> &blob) noexcept;
    // Writes a blob value to NVS.
    esp_err_t writeBlob(const char *key, const void *value, size_t valueLen) noexcept;

    // Reads a 32-bit integer value from NVS.
    esp_err_t readInt(const char *key, int32_t *pValue) noexcept;
    // Writes a 32-bit integer value to NVS.
    esp_err_t writeInt(const char *key, int32_t value) noexcept;

    // Erases a single key from NVS.
    esp_err_t erase(const char *key) noexcept;
    // Erases and reinitializes the whole NVS partition.
    void eraseAll() noexcept;

    // Commits pending writes to flash.
    esp_err_t commit() noexcept;

private:
    esp_err_t init() noexcept;
    esp_err_t open(bool readOnly) noexcept;

    esp_err_t convertError(esp_err_t err) noexcept;

private:
    const char *nameSpace{nullptr};
    const char *partition{nullptr};
    nvs_handle_t handle{0};
    bool readOnlyMode{false};
};

#endif // __cplusplus
