#pragma once

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include "allocator.h"
#include <assert.h>
#include <cstring>

// -----------------------------------------------------------------------------

namespace lightstd {

// NOTE: lightstd::string does NOT throw exceptions on purpose.
class string
{
public:
    using iterator        = char*;
    using const_iterator  = const char*;

    string(IAllocator *_alloc = nullptr) noexcept
    {
        alloc = _alloc ? _alloc : IAllocator::getDefault();
    }

    string(const string&) = delete;
    string(string&& other) noexcept : ptr(other.ptr), len(other.len), cap(other.cap), alloc(other.alloc)
    {
        other.ptr = nullptr;
        other.len = 0;
        other.cap = 0;
    }

    ~string()
    {
        if (ptr) {
            alloc->deallocate(ptr);
        }
    }

    string& operator=(const string&) = delete;
    string& operator=(string&& other) noexcept
    {
        if (this != &other) {
            ptr = other.ptr;
            len = other.len;
            cap = other.cap;
            alloc = other.alloc;
            other.ptr = nullptr;
            other.len = 0;
            other.cap = 0;
        }
        return *this;
    }

    [[nodiscard]] size_t length() const noexcept
    {
        return len;
    }

    [[nodiscard]] size_t capacity() const noexcept
    {
        return cap;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return len == 0;
    }

    char* data() noexcept
    {
        assert(ptr);
        return ptr;
    }

    const char* data() const noexcept
    {
        return c_str();
    }

    const char* c_str() const noexcept
    {
        static const char empty[1] = "";
        return ptr ? ptr : empty;
    }

    operator const char*() const noexcept
    {
        return c_str();
    }

    iterator begin() noexcept
    {
        return ptr;
    }

    const_iterator begin() const noexcept
    {
        return ptr;
    }

    const_iterator cbegin() const noexcept
    {
        return ptr;
    }

    iterator end() noexcept
    {
        return ptr + len;
    }

    const_iterator end() const noexcept
    {
        return ptr + len;
    }

    const_iterator cend() const noexcept
    {
        return ptr + len;
    }

    [[nodiscard]] char& operator[](size_t idx) noexcept
    {
        assert(ptr);
        assert(idx < len);
        return ptr[idx]; // WARNING: no bounds check
    }

    [[nodiscard]] const char& operator[](size_t idx) const noexcept
    {
        assert(ptr);
        assert(idx < len);
        return ptr[idx];
    }

    explicit operator bool() const noexcept
    {
        return len != 0;
    }

    bool operator!() const noexcept
    {
        return len == 0;
    }

    void clear() noexcept
    {
        len = 0;
        if (ptr) {
            ptr[0] = '\0';
        }
    }

    bool append(const char* src) noexcept
    {
        size_t len;

        if (!src) {
            return true;
        }
        for (len = 0; src[len] != '\0'; ++len);
        return append(src, len);
    }

    bool append(const char* src, size_t srcLen) noexcept
    {
        if ((!src) || srcLen == 0) {
            return true;
        }

        if (!reserve(len + srcLen)) {
            return false;
        }

        memcpy(ptr + len, src, srcLen);
        len += srcLen;
        ptr[len] = '\0';
        return true;
    }

    bool push_back(char c) noexcept
    {
        return append(&c, 1);
    }

    // Reserve space for at least newCapacity characters (excluding '\0')
    [[nodiscard]] bool reserve(size_t newCapacity) noexcept
    {
        char* newPtr;

        if (newCapacity <= cap) {
            return true;
        }

        // Allocate
        newPtr = (char *)alloc->allocate(newCapacity + 1); // extra room for nul character
        if (!newPtr) {
            return false;
        }
        if (ptr) {
            memcpy(newPtr, ptr, len);
            alloc->deallocate(ptr);
        }
        else {
            len = 0;
        }
        ptr = newPtr;
        cap = newCapacity;
        ptr[len] = '\0';
        return true;
    }

    [[nodiscard]] bool resize(size_t newLen) noexcept
    {
        if (newLen > cap) {
            if (!reserve(newLen)) {
                return false;
            }
        }
        else if (cap == 0) {
            // Try to allocate some room if the string is empty
            // regardless we are not adding anything to the string
            if (!reserve(16)) {
                return false;
            }
        }
        len = newLen;
        if (ptr) {
            ptr[len] = '\0';
        }
        return true;
    }

private:
    char* ptr{nullptr};
    size_t len{0};
    size_t cap{0};
    IAllocator *alloc{nullptr};
};

} // namespace lightstd
