#pragma once

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include "../lightstd/string.h"
#include "../lightstd/vector.h"
#include <esp_err.h>

// -----------------------------------------------------------------------------

// Base interface for key-value storage.
class IStorage
{
public:
    virtual esp_err_t readStr(const char *key, lightstd::string &str) = 0;
    virtual esp_err_t writeStr(const char *key, const char *value) = 0;

    virtual esp_err_t readBlob(const char *key, lightstd::vector<uint8_t> &blob) = 0;
    virtual esp_err_t writeBlob(const char *key, const void *value, size_t valueLen) = 0;

    virtual esp_err_t readInt(const char *key, int32_t *pValue) = 0;
    virtual esp_err_t writeInt(const char *key, int32_t value) = 0;

    virtual esp_err_t erase(const char *key) = 0;
};
