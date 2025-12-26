#pragma once

#include "allocator.h"

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

namespace lightstd {

template<typename T>
class auto_ptr
{
public:
    constexpr auto_ptr(IAllocator *_alloc = nullptr) noexcept
    {
        alloc = _alloc ? _alloc : IAllocator::get_default();
    }

    explicit auto_ptr(const T* _ptr, IAllocator *_alloc = nullptr) noexcept
    {
        alloc = _alloc ? _alloc : IAllocator::get_default();
        ptr = _ptr;
    }

    auto_ptr(const auto_ptr& other) = delete;
    auto_ptr& operator=(const auto_ptr&) = delete;

    auto_ptr(auto_ptr&& other) noexcept : ptr(other.ptr), alloc(other.alloc)
    {
        other.ptr = nullptr;
    }

    ~auto_ptr()
    {
        alloc->deallocate(ptr);
    }

    auto_ptr& operator=(auto_ptr&& other) noexcept
    {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    void reset() noexcept
    {
        alloc->deallocate(ptr);
        ptr = nullptr;
    }

    bool allocate() noexcept
    {
        return allocateWithSize(sizeof(T));
    }

    bool allocateWithSize(size_t size = 0) noexcept
    {
        alloc->deallocate(ptr);
        ptr = (T *)alloc->allocate(size);
        return !!ptr;
    }

    void attach(const T *_ptr)
    {
        alloc->deallocate(ptr);
        ptr = _ptr;
    }

    T* detach()
    {
        T *_ptr = ptr;
        ptr = nullptr;
        return _ptr;
    }

    T* get() const noexcept
    {
        assert(ptr);
        return ptr;
    }

    T& operator*() const
    {
        assert(ptr);
        return *ptr;
    }

    T* operator->() const noexcept
    {
        assert(ptr);
        return ptr;
    }

    operator const T*() const noexcept
    {
        assert(ptr);
        return ptr;
    }

    operator T*() noexcept
    {
        assert(ptr);
        return ptr;
    }

    explicit operator bool() const noexcept
    {
        return ptr != nullptr;
    }

    bool operator!() const noexcept
    {
        return ptr == nullptr;
    }

    void swap(auto_ptr& other) noexcept
    {
        T* other_ptr = other.ptr;
        other.ptr = ptr;
        ptr = other_ptr;
    }

private:
    T *ptr{nullptr};
    IAllocator *alloc;
};

} // namespace lightstd
