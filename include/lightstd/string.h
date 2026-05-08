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

    // Creates an empty string using the provided allocator or the default one.
    string(IAllocator *_alloc = nullptr) noexcept
    {
        alloc = _alloc ? _alloc : IAllocator::getDefault();
    }

    string(const string&) = delete;
    // Transfers ownership of the string buffer.
    string(string&& other) noexcept : ptr(other.ptr), len(other.len), cap(other.cap), alloc(other.alloc)
    {
        other.ptr = nullptr;
        other.len = 0;
        other.cap = 0;
    }

    // Releases the owned character buffer.
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
            if (ptr) {
                alloc->deallocate(ptr);
            }

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

    // Returns the number of characters excluding the trailing nul.
    [[nodiscard]] size_t length() const noexcept
    {
        return len;
    }

    // Returns the allocated character capacity excluding the trailing nul.
    [[nodiscard]] size_t capacity() const noexcept
    {
        return cap;
    }

    // Reports whether the string contains no characters.
    [[nodiscard]] bool empty() const noexcept
    {
        return len == 0;
    }

    // Returns mutable access to the contiguous character buffer.
    char* data() noexcept
    {
        assert(ptr);
        return ptr;
    }

    // Returns read-only access to the contiguous character buffer.
    const char* data() const noexcept
    {
        return c_str();
    }

    // Returns a nul-terminated view of the string contents.
    const char* c_str() const noexcept
    {
        static const char empty[1] = "";
        return ptr ? ptr : empty;
    }

    // Implicitly exposes the string as a nul-terminated C string.
    operator const char*() const noexcept
    {
        return c_str();
    }

    // Returns an iterator to the first character.
    iterator begin() noexcept
    {
        return ptr;
    }

    // Returns a const iterator to the first character.
    const_iterator begin() const noexcept
    {
        return ptr;
    }

    // Returns a const iterator to the first character.
    const_iterator cbegin() const noexcept
    {
        return ptr;
    }

    // Returns an iterator one past the last character.
    iterator end() noexcept
    {
        return ptr + len;
    }

    // Returns a const iterator one past the last character.
    const_iterator end() const noexcept
    {
        return ptr + len;
    }

    // Returns a const iterator one past the last character.
    const_iterator cend() const noexcept
    {
        return ptr + len;
    }

    // Returns a reference to the character at the requested index.
    [[nodiscard]] char& operator[](size_t idx) noexcept
    {
        assert(ptr);
        assert(idx < len);
        return ptr[idx]; // WARNING: no bounds check
    }

    // Returns a read-only reference to the character at the requested index.
    [[nodiscard]] const char& operator[](size_t idx) const noexcept
    {
        assert(ptr);
        assert(idx < len);
        return ptr[idx];
    }

    // Reports whether the string contains at least one character.
    explicit operator bool() const noexcept
    {
        return len != 0;
    }

    // Reports whether the string is empty.
    bool operator!() const noexcept
    {
        return len == 0;
    }

    // Removes all characters while keeping the current allocation.
    void clear() noexcept
    {
        len = 0;
        if (ptr) {
            ptr[0] = '\0';
        }
    }

    // Appends a nul-terminated string.
    bool append(const char* src) noexcept
    {
        size_t len;

        if (!src) {
            return true;
        }
        for (len = 0; src[len] != '\0'; ++len);
        return append(src, len);
    }

    // Appends a byte range without requiring a trailing nul.
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

    // Appends a single character.
    bool push_back(char c) noexcept
    {
        return append(&c, 1);
    }

    // Ensures capacity for at least the requested character count.
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

    // Resizes the string and preserves nul termination.
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
