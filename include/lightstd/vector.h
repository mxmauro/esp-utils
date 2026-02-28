#pragma once

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include "allocator.h"
#include <assert.h>
#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------

namespace lightstd {

// NOTE: lightstd::vector does NOT throw exceptions on purpose.
template <class T>
class vector final
{
public:
    using iterator        = T*;
    using const_iterator  = const T*;

    vector(IAllocator *_alloc = nullptr) noexcept
    {
        alloc = _alloc ? _alloc : IAllocator::getDefault();
    }

    vector(const vector&) noexcept = delete;
    vector(vector&& other) noexcept
    {
        move_from(other);
    }

    vector& operator=(const vector&) noexcept = delete;
    vector& operator=(vector&& other) noexcept
    {
        if (this != &other) {
            destroy_and_deallocate();
            move_from(other);
        }
        return *this;
    }

    ~vector() noexcept
    {
        destroy_and_deallocate();
    }

    [[nodiscard]] size_t size() const noexcept
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

    T* data() noexcept
    {
        return ptr;
    }

    const T* data() const noexcept
    {
        return ptr;
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

    [[nodiscard]] T& operator[](size_t idx) noexcept
    {
        assert(idx < len);
        return ptr[idx]; // WARNING: no bounds check
    }

    [[nodiscard]] const T& operator[](size_t idx) const noexcept
    {
        assert(idx < len);
        return ptr[idx];
    }

    [[nodiscard]] T& front() noexcept
    {
        assert(len > 0);
        return ptr[0];
    }

    [[nodiscard]] const T& front() const noexcept
    {
        assert(len > 0);
        return ptr[0];
    }

    [[nodiscard]] T& back() noexcept
    {
        assert(len > 0);
        return ptr[len - 1];
    }

    [[nodiscard]] const T& back() const noexcept
    {
        assert(len > 0);
        return ptr[len - 1];
    }

    void clear() noexcept
    {
        destroy_range(0, len);
        len = 0;
    }

    void pop_back() noexcept
    {
        assert(len > 0);
        len--;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            ptr[len].~T();
        }
        ptr[len].~T();
    }

    [[nodiscard]] bool reserve(size_t newCapacity) noexcept
    {
        if (newCapacity <= cap) {
            return true;
        }
        return reallocate(newCapacity, false);
    }

    [[nodiscard]] bool shrink_to_fit() noexcept
    {
        if (len == cap) {
            return true;
        }
        return reallocate(len, true);
    }

    [[nodiscard]] bool resize(size_t newLen) noexcept
    {
        static_assert(std::is_nothrow_default_constructible_v<T>,
                    "resize(grow) requires T to be nothrow default constructible.");

        if (newLen == len) {
            return true;
        }

        if (newLen < len) {
            destroy_range(newLen, len);
            len = newLen;
            return true;
        }

        // grow
        if (newLen > cap) {
            size_t target = growth_capacity(newLen);
            if (!reallocate(target, false)) {
                return false;
            }
        }

        // default-construct new elements
        for (size_t i = len; i < newLen; ++i) {
            ::new (static_cast<void*>(ptr + i)) T();
        }

        len = newLen;
        return true;
    }

    [[nodiscard]] bool resize(size_t newLen, const T& fillValue) noexcept
    {
        static_assert(std::is_nothrow_copy_constructible_v<T>,
                      "resize(n, value) requires T to be nothrow copy constructible.");

        if (newLen == len) {
            return true;
        }

        if (newLen < len) {
            destroy_range(newLen, len);
            len = newLen;
            return true;
        }

        // grow
        if (newLen > cap) {
             size_t target = growth_capacity(newLen);

            // Determine alias *before* reallocation moves the buffer.
            // If fillValue refers to an element inside the old buffer, capture its
            // index so we can locate it in the new buffer after reallocation.
            const bool aliases = (ptr && &fillValue >= ptr && &fillValue < ptr + len);
            const size_t aliasIdx = aliases ? static_cast<size_t>(&fillValue - ptr) : 0;

            if (!reallocate(target, false)) {
                return false;
            }

            // After reallocation ptr points to the new buffer.
            // If fillValue was in the old buffer, use its new location; otherwise use directly.
            const T& src = aliases ? ptr[aliasIdx] : fillValue;
            for (size_t i = len; i < newLen; ++i) {
                ::new (static_cast<void*>(ptr + i)) T(src);
            }
        }
        else {
            for (size_t i = len; i < newLen; ++i) {
                ::new (static_cast<void*>(ptr + i)) T(fillValue);
            }
        }

        len = newLen;
        return true;
    }

    [[nodiscard]] bool push_back(const T& v) noexcept
    {
        static_assert(std::is_nothrow_copy_constructible_v<T>,
                      "push_back(const T&) requires T to be nothrow copy constructible.");

        // Check if v aliases an element inside our buffer. If reallocation is needed
        // and v points into ptr, it would dangle after the move. In that case copy
        // first, then reallocate. If no reallocation is needed, construct directly.
        const bool aliases = (len == cap && ptr && &v >= ptr && &v < ptr + len);

        if (aliases) {
            T vCopy(v); // copy before reallocation invalidates the reference

            if (!ensure_capacity_for_one_more()) {
                return false;
            }

            ::new (static_cast<void*>(ptr + len)) T(std::move(vCopy));
        } else {
            if (!ensure_capacity_for_one_more()) {
                return false;
            }
            ::new (static_cast<void*>(ptr + len)) T(v);
        }

        len++;
        return true;
    }

    [[nodiscard]] bool push_back(T&& v) noexcept
    {
        static_assert(std::is_nothrow_move_constructible_v<T>,
                      "push_back(const T&) requires T to be nothrow move constructible.");

        if (!ensure_capacity_for_one_more()) {
            return false;
        }

        ::new (static_cast<void*>(ptr + len)) T(std::move(v));

        len++;
        return true;
    }

    template <class... Args>
    [[nodiscard]] bool emplace_back(Args&&... args) noexcept
    {
        static_assert(std::is_nothrow_constructible_v<T, Args...>,
                      "emplace_back requires T(args...) to be nothrow constructible.");

        if (!ensure_capacity_for_one_more()) {
            return false;
        }

        ::new (static_cast<void*>(ptr + len)) T(std::forward<Args>(args)...);

        len++;
        return true;
    }

    // Optional: erase last N elements without realloc
    void resize_down(size_t newLen) noexcept
    {
        assert(newLen <= len);
        destroy_range(newLen, len);
        len = newLen;
    }

private:
    static constexpr size_t max_array_size() noexcept
    {
        return ((size_t)-1) / sizeof(T);
    }

    [[nodiscard]] size_t growth_capacity(size_t minNeeded) const noexcept
    {
        size_t c = (cap < 16) ? 16 : cap;

        while (c < minNeeded) {
            if (c > max_array_size() / 2) {
                return minNeeded;
            }
            c *= 2;
        }
        return c;
    }

    [[nodiscard]] bool ensure_capacity_for_one_more() noexcept
    {
        if (len < cap) {
            return true;
        }
        if (len == max_array_size()) {
            return false;
        }
        return reallocate(growth_capacity(len + 1), false);
    }

    [[nodiscard]] bool reallocate(size_t newCapacity, bool force) noexcept
    {
        T* newPtr;

        static_assert(std::is_nothrow_move_constructible_v<T>,
              "vector requires T to be nothrow move constructible for reallocation.");

        if (!force && newCapacity <= cap) {
            return true;
        }
        if (newCapacity == cap) {
            return true; // nothing to do / or assert
        }
        if (newCapacity > max_array_size()) {
            return false;
        }

        // Nothing to (re)allocate — just free existing storage.
        if (newCapacity == 0) {
            if (ptr) {
                alloc->deallocate(ptr);
                ptr = nullptr;
                cap = 0;
            }
            return true;
        }

        // Allocate raw storage (nothrow)
        newPtr = static_cast<T*>(alloc->allocate(newCapacity * sizeof(T)));
        if (!newPtr) {
            return false;
        }

        // Only move elements that will survive the resize. Elements beyond
        // newCapacity are simply left in the old buffer and destroyed there.
        size_t newLen = (len < newCapacity) ? len : newCapacity;

        if constexpr (std::is_trivially_copyable_v<T>) {
            // For trivial types (int, float, pointers, POD structs) a single memcpy
            // is correct and significantly faster than element-wise move construction.
            if (newLen > 0) {
                memcpy(newPtr, ptr, newLen * sizeof(T));
            }
        } else {
            for (size_t i = 0; i < newLen; ++i) {
                ::new (static_cast<void*>(newPtr + i)) T(std::move(ptr[i]));
            }
        }

        // Destroy old elements + free old storage
        destroy_range(0, len);

        if (ptr) {
            alloc->deallocate(ptr);
        }

        ptr = newPtr;
        cap  = newCapacity;
        len = newLen;
        return true;
    }

    void destroy_range(size_t from, size_t to) noexcept
    {
        assert(ptr || from == to);

        // Trivially destructible types (int, float, pointers, etc.) need no destructor call.
        if constexpr (!std::is_trivially_destructible_v<T>) {
            if (ptr) {
                // Destroy in reverse order [to-1 .. from] to mirror construction order.
                for (size_t i = to; i > from; --i) {
                    ptr[i - 1].~T();
                }
            }
        }
    }

    void destroy_and_deallocate() noexcept
    {
        if (ptr) {
            destroy_range(0, len);
            alloc->deallocate(ptr);
            ptr = nullptr;
            len = 0;
            cap  = 0;
        }
    }

    void move_from(vector& other) noexcept
    {
        ptr = other.ptr;
        len = other.len;
        cap  = other.cap;
        alloc = other.alloc;
        other.ptr = nullptr;
        other.len = 0;
        other.cap  = 0;
    }

private:
    T* ptr{nullptr};
    size_t len{0};
    size_t cap{0};
    IAllocator *alloc{nullptr};
};

} // namespace lightstd
