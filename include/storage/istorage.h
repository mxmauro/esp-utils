#pragma once

#include "../lightstd/auto_ptr.h"
#include "../lightstd/string.h"
#include <esp_err.h>

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

typedef struct StorageBlob_s {
    lightstd::auto_ptr<uint8_t> value;
    size_t len{0};
} StorageBlob_t;

// Base interface for key-value storage.
class IStorage
{
public:
    virtual esp_err_t readStr(const char *key, lightstd::string &str) = 0;
    virtual esp_err_t writeStr(const char *key, const char *value) = 0;

    virtual esp_err_t readBlob(const char *key, StorageBlob_t &blob) = 0;
    virtual esp_err_t writeBlob(const char *key, const void *value, size_t valueLen) = 0;

    virtual esp_err_t readInt(const char *key, int32_t *pValue) = 0;
    virtual esp_err_t writeInt(const char *key, int32_t value) = 0;

    virtual esp_err_t erase(const char *key) = 0;
};
