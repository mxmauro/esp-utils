#pragma once

#include "allocator.h"
#include <memory.h>

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

namespace lightstd {

// NOTE: lightstd::string does NOT throw exceptions on purpose.
//       Always check the value of hasError before assuming the string
//       contains the expected value.
class string
{
public:
    constexpr string(IAllocator *_alloc = nullptr) noexcept
    {
        alloc = _alloc ? _alloc : IAllocator::get_default();
    }

    explicit string(const char* src, IAllocator *_alloc = nullptr) noexcept
    {
        alloc = _alloc ? _alloc : IAllocator::get_default();
        if (src) {
            size_t srcLen = strlen(src);
            if (reserve(len)) {
                memcpy(ptr, src, srcLen);
                len = srcLen;
                ptr[len] = '\0';
            }
        }
    }

    string(const string& other) noexcept : alloc(other.alloc)
    {
        if (other.len > 0) {
            if (reserve(other.len)) {
                memcpy(ptr, other.ptr, other.len + 1); // +1 for '\0'
                len = other.len;
            }
        }
    }

    string(string&& other) noexcept : ptr(other.ptr), len(other.len), size(other.size), alloc(other.alloc)
    {
        other.ptr = nullptr;
        other.len = 0;
        other.size = 0;
    }

    ~string()
    {
        alloc->deallocate(ptr);
    }

    string& operator=(const string& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        if (other.len == 0) {
            clear();
            return *this;
        }
        if (reserve(other.len)) {
            memcpy(ptr, other.ptr, other.len);
            len = other.len;
            ptr[len] = 0;
        }
        return *this;
    }

    string& operator=(string&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        alloc->deallocate(ptr);
        ptr = other.ptr;
        len = other.len;
        size = other.size;
        other.ptr = nullptr;
        other.len = 0;
        other.size = 0;
        return *this;
    }

    string& operator=(const char *src) noexcept
    {
        size_t srcLen = src ? strlen(src) : 0;
        if (srcLen == 0) {
            clear();
            return *this;
        }
        if (reserve(srcLen)) {
            memcpy(ptr, src, srcLen);
            len = srcLen;
            ptr[len] = 0;
        }
        return *this;
    }

    bool append(const char* src) noexcept
    {
        if (!src) {
            return !badAlloc;
        }
        return append(src, strlen(src));
    }

    bool append(const char* src, size_t srcLen) noexcept
    {
        if ((!src) || srcLen == 0) {
            return !badAlloc;
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

    // Reserve space for at least newSize characters (excluding '\0')
    bool reserve(size_t newSize) noexcept
    {
        char* newPtr;

        if (badAlloc) {
            return false;
        }

        if (newSize <= size) {
            if (ptr) {
                ptr[newSize] = 0;
            }
            return !badAlloc;
        }

        // Allocate
        newPtr = (char *)alloc->allocate(newSize + 1); // extra room for nul character
        if (!newPtr) {
            badAlloc = true;
            return false;
        }
        if (!ptr) {
            len = 0;
        }
        else {
            memcpy(newPtr, ptr, size);
            alloc->deallocate(ptr);
        }
        ptr = newPtr;
        size = newSize;
        ptr[len] = '\0';
        return true;
    }

    bool resize(size_t newLen) noexcept
    {
        if (badAlloc) {
            return false;
        }

        if (newLen > size) {
            if (!reserve(newLen)) {
                return false;
            }
        }
        len = newLen;
        if (ptr) {
            ptr[len] = '\0';
        }
        return true;
    }

    void clear() noexcept
    {
        len = 0;
        if (ptr) {
            ptr[0] = '\0';
        }
    }

    bool hasError() const noexcept
    {
        return badAlloc;
    }

    size_t length() const noexcept
    {
        return len;
    }

    size_t capacity() const noexcept
    {
        return size;
    }

    bool empty() const noexcept
    {
        return len == 0;
    }

    char* data() noexcept
    {
        static char empty[2] = "\0";
        ensure_c_str_allocated();
        return ptr ? ptr : empty;
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

    char& operator[](size_t idx) noexcept
    {
        assert(ptr);
        assert(idx < size);
        return ptr[idx]; // WARNING: no bounds check
    }

    const char& operator[](size_t idx) const noexcept
    {
        assert(ptr);
        assert(idx < size);
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

private:
    void ensure_c_str_allocated()
    {
        if ((!ptr) && (!badAlloc)) {
            // minimal 1-byte buffer for empty string
            ptr = (char *)alloc->allocate(1);
            if (ptr) {
                ptr[0] = '\0';
                len = 0;
                size = 0;
            }
            else {
                badAlloc = true;
            }
        }
    }

private:
    char* ptr{nullptr};
    size_t len{0};
    size_t size{0};
    bool badAlloc{false};
    IAllocator *alloc;
};

} // namespace lightstd
