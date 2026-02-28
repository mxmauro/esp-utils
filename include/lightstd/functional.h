#pragma once

// A lightweight std::function replacement for ESP-IDF.
//
// Design goals:
//   - No exceptions (ESP-IDF is built with -fno-exceptions)
//   - No RTTI required
//   - Small Buffer Optimization (SBO): callables <= SboSize bytes are stored
//     inline, avoiding heap allocation for the common case (lambdas with
//     a few captures, plain function pointers, etc.)
//   - Correct value semantics: callable is owned (copied/moved into buffer)
//   - Correct copy/move via a vtable of plain function pointers
//
// Template parameters:
//   Signature  — the call signature, e.g. void(int, float)
//   SboSize    — inline storage capacity in bytes (default 32).
//                Tune per call-site if you know the callable is larger or if
//                you want to save RAM:
//                  light_function<void(), 64> f = big_lambda;  // 64-byte SBO
//                  light_function<void(),  8> f = []{ tick(); }; // tiny SBO
//                NOTE: SboSize cannot be derived from the call signature —
//                Args describe what the function *accepts*, not what it
//                *captures*. A light_function<void(int)> could store a lambda
//                capturing 128 bytes; the SBO must be an independent constant.
//
// Usage:
//   light_function<void(int)> fn = [x](int v) { do_thing(v + x); };
//   fn(42);
//   if (fn) { ... }
//   fn = nullptr; // reset
//
// Error policy (no-exceptions build):
//   - Calling an empty light_function  → abort()
//   - Heap allocation failure           → abort()
//   Both produce a clear panic + backtrace on ESP-IDF.

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include <cstddef>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------

namespace lightstd {

// Forward declaration of primary template
template<typename Signature, std::size_t SboSize = 32>
class light_function;

// Partial specialisation for callable signatures
template<typename R, typename... Args, std::size_t SboSize>
class light_function<R(Args...), SboSize>
{
public:
    static constexpr std::size_t SBO_SIZE  = SboSize;
    static constexpr std::size_t SBO_ALIGN = alignof(std::max_align_t);

    // SboSize == 0 would produce a zero-length array (a GCC extension, not
    // standard C++) and silently route every callable through the heap.
    static_assert(SboSize > 0, "light_function: SboSize must be at least 1");

private:
    // Vtable: plain function pointers — no virtual dispatch, no RTTI.
    // alloc_size carries sizeof(F)
    struct Vtable
    {
        R           (*invoke)       (void* storage, Args&&... args);
        void        (*copy)         (void* dst, const void* src);
        void        (*move_destroy) (void* dst, void* src); // move dst, destroy src
        void        (*destroy)      (void* storage);
        std::size_t alloc_size;   // sizeof(F) — used when heap path is taken
    };

    template<typename F>
    static const Vtable* vtable_for() noexcept
    {
        static const Vtable vt = {
            // invoke — forward each argument to preserve move semantics
            [](void* s, Args&&... args) -> R {
                return (*static_cast<F*>(s))(std::forward<Args>(args)...);
            },
            // copy-construct dst from src
            [](void* dst, const void* src) {
                ::new (dst) F(*static_cast<const F*>(src));
            },
            // move-construct dst from src, then destroy src in-place
            [](void* dst, void* src) {
                ::new (dst) F(std::move(*static_cast<F*>(src)));
                static_cast<F*>(src)->~F();
            },
            // destroy in-place
            [](void* s) {
                static_cast<F*>(s)->~F();
            },
            sizeof(F)   // alloc_size
        };
        return &vt;
    }

    // Storage
    alignas(SBO_ALIGN) unsigned char m_sbo[SBO_SIZE];

    void*         m_heap{nullptr};   // non-null => callable lives on the heap
    const Vtable* m_vtable{nullptr}; // null => empty

    void* active_storage() noexcept
    {
         return m_heap ? m_heap : m_sbo;
    }
    const void* active_storage() const noexcept
    {
        return m_heap ? m_heap : m_sbo;
    }

    // Tag-dispatch helpers: SBO vs heap allocation.
    // Overloads prevent the compiler from instantiating the placement-new
    // branch with a type that doesn't fit (avoids -Wplacement-new / UB).
    template<typename Fd, typename F>
    void do_store(F&& f, std::true_type /*fits in SBO*/) noexcept
    {
        // Noexcept is unconditional here: the static_asserts in do_assign
        // guarantee Fd is both nothrow-copy and nothrow-move constructible,
        // so the placement-new cannot throw regardless of value category.
        m_heap = nullptr;
        ::new (static_cast<void*>(m_sbo)) Fd(std::forward<F>(f));
    }

    template<typename Fd, typename F>
    void do_store(F&& f, std::false_type /*too large — use heap*/)
    {
        // Use std::nothrow: under -fno-exceptions the default ::operator new
        // may still invoke the new-handler or behave implementation-defined.
        // std::nothrow guarantees a nullptr return on failure.
        void* mem = ::operator new(sizeof(Fd), std::nothrow);
        if (!mem)
        {
            // Out of heap memory — unrecoverable on ESP-IDF.
            abort();
        }
        m_heap = mem;
        ::new (m_heap) Fd(std::forward<F>(f));
    }

    template<typename F>
    void do_assign(F&& f) noexcept
    {
        using Fd = typename std::decay<F>::type;
        using fits = std::integral_constant<bool, sizeof(Fd) <= SBO_SIZE && alignof(Fd) <= SBO_ALIGN>;

        // Compile-time safety contracts:
        //
        // [1] Nothrow move constructor (required)
        //
        //     do_move_from() and the move-assignment operator are marked
        //     noexcept. In the SBO path they invoke Fd's move constructor at
        //     runtime through the vtable. If that move constructor throws,
        //     noexcept converts the exception into std::terminate with no
        //     useful diagnostic. This assert catches the problem at compile
        //     time instead.
        //
        //     In practice this never fires for well-written code: every
        //     standard-library type (std::string, std::vector, std::shared_ptr,
        //     …) has a noexcept move constructor by design. The rule is simple:
        //     if your move constructor cannot throw, mark it noexcept.
        static_assert(
            std::is_nothrow_move_constructible<Fd>::value,
            "light_function: stored callable must be nothrow move constructible. "
            "All standard lambdas and well-designed functors satisfy this. "
            "If your type has a throwing move ctor, wrap it in std::unique_ptr."
        );

        // [2] Nothrow copy constructor (required)
        //
        //     do_copy_from() is called from the copy constructor and copy-
        //     assignment operator, neither of which is noexcept. So a throwing
        //     copy constructor would propagate cleanly without hitting
        //     std::terminate. Why assert then?
        //
        //     Because on ESP-IDF (-fno-exceptions) ANY throw becomes
        //     std::terminate. A callable whose copy constructor throws would
        //     silently terminate the firmware with no backtrace and no warning
        //     at the call site. The assert makes the contract explicit and
        //     catches the problem at compile time.
        //
        //     Common concern — "what about lambdas capturing std::string?"
        //     The answer depends on HOW you capture:
        //
        //       std::string name = "foo";
        //
        //       // Capture by VALUE — lambda stores a full copy of 'name'.
        //       // Copying the lambda copies the string → can allocate → NOT nothrow.
        //       // This assert WILL fire. Store by ref or move into the lambda instead.
        //       light_function<void()> f = [name]{ use(name); };   // ← fires
        //
        //       // Capture by REFERENCE — lambda stores only a pointer to 'name'.
        //       // Copying the lambda copies a pointer → always nothrow.
        //       // This assert will NOT fire. Caller must ensure 'name' outlives f.
        //       light_function<void()> f = [&name]{ use(name); };  // ← OK
        //
        //       // Move into capture — lambda owns the string, but the lambda's
        //       // own copy constructor is deleted (move-only lambda).
        //       // light_function's copy ctor/operator= won't compile for this
        //       // callable anyway — caught earlier by the copy vtable entry.
        //       light_function<void()> f = [n=std::move(name)]{ use(n); };
        //
        //     If you genuinely need a copyable lambda that owns a std::string,
        //     wrap the string in a std::shared_ptr<std::string>: the shared_ptr
        //     copy is nothrow and both lambda copies share the same string.
        static_assert(
            std::is_nothrow_copy_constructible<Fd>::value,
            "light_function: stored callable must be nothrow copy constructible. "
            "Capture heavy types by reference [&x] instead of by value [x], "
            "or wrap shared state in std::shared_ptr."
        );

        do_store<Fd>(std::forward<F>(f), fits{});
        m_vtable = vtable_for<Fd>();
    }

    void do_destroy() noexcept
    {
        if (m_vtable)
        {
            m_vtable->destroy(active_storage());
            if (m_heap)
            {
                ::operator delete(m_heap);
                m_heap = nullptr;
            }
            m_vtable = nullptr;
        }
    }

    void do_copy_from(const light_function& other) noexcept
    {
        if (!other.m_vtable) return;

        if (other.m_heap)
        {
            // Null-check the allocation; abort() on failure (heap exhausted).
            void* mem = ::operator new(other.m_vtable->alloc_size, std::nothrow);
            if (!mem) abort();

            // The static_asserts in do_assign guarantee the stored callable is
            // nothrow copy constructible, so vtable->copy() below cannot throw
            // and this guard will never actually trigger. It is kept as a
            // defensive safety net in case the class is ever modified to relax
            // the copy constraint, and to document the intended cleanup logic.
            struct HeapGuard {
                void* ptr;
                bool  committed;
                ~HeapGuard() { if (!committed) ::operator delete(ptr); }
            } guard{mem, false};

            other.m_vtable->copy(mem, other.m_heap);
            guard.committed = true;
            m_heap = mem;
        }
        else
        {
            m_heap = nullptr;
            other.m_vtable->copy(m_sbo, other.m_sbo);
        }

        // Set vtable last — consistent with do_assign ordering.
        m_vtable = other.m_vtable;
    }

    // do_move_from is noexcept because:
    //   • Heap path:  pointer steal only — no construction, no throw.
    //   • SBO path:   calls F's move constructor via move_destroy.
    //                 If that move constructor throws (pathological — well-
    //                 designed types have noexcept move ctors), noexcept
    //                 causes std::terminate.  This matches the contract
    //                 std::function imposes and is acceptable because:
    //                   1. Move constructors that throw are an anti-pattern.
    //                   2. On -fno-exceptions builds this path is unreachable.
    //                 If you need to store a type with a throwing move ctor,
    //                 wrap it in std::unique_ptr so the pointer (noexcept move)
    //                 is what gets stored.
    void do_move_from(light_function& other) noexcept
    {
        if (!other.m_vtable) return;

        if (other.m_heap)
        {
            // Steal the heap pointer — no allocation needed.
            m_heap = other.m_heap;
            other.m_heap = nullptr;
        }
        else
        {
            m_heap = nullptr;
            // Move-construct into our SBO, then destroy the source SBO object.
            other.m_vtable->move_destroy(m_sbo, other.m_sbo);
        }

        m_vtable       = other.m_vtable;
        other.m_vtable = nullptr;
    }

public:
    light_function() noexcept = default;
    light_function(std::nullptr_t) noexcept {}

    // Construct from any callable (lambda, functor, plain function pointer).
    // SFINAE prevents this from shadowing the copy/move constructors.
    // NOTE: a light_function with a *different* SboSize is not excluded —
    // it will be stored as a nested callable (same as std::function wrapping
    // another std::function). This is intentional but can be surprising;
    // prefer assigning same-SboSize instances when possible.
    template<
        typename F,
        typename = typename std::enable_if<
            !std::is_same<typename std::decay<F>::type, light_function>::value
        >::type
    >
    light_function(F&& f) noexcept
    {
        do_assign(std::forward<F>(f));
    }

    light_function(const light_function& other) noexcept
    {
        do_copy_from(other);
    }

    light_function(light_function&& other) noexcept
    {
        do_move_from(other);
    }

    ~light_function()
    {
        do_destroy();
    }

    light_function& operator=(const light_function& other) noexcept
    {
        if (this != &other)
        {
            do_destroy();
            do_copy_from(other);
        }
        return *this;
    }

    light_function& operator=(light_function&& other) noexcept
    {
        if (this != &other)
        {
            do_destroy();
            do_move_from(other);
        }
        return *this;
    }

    light_function& operator=(std::nullptr_t) noexcept
    {
        do_destroy();
        return *this;
    }

    template<
        typename F,
        typename = typename std::enable_if<
            !std::is_same<typename std::decay<F>::type, light_function>::value
        >::type
    >
    light_function& operator=(F&& f) noexcept
    {
        do_destroy();
        do_assign(std::forward<F>(f));
        return *this;
    }

    // Invocation.
    //
    // Takes Args... by value, not Args&&...:
    //   - Args&&... would refuse to bind lvalue arguments for value types,
    //     e.g. light_function<int(int)> called as f(x) where x is an int
    //     lvalue would fail to compile (can't bind int&& to int lvalue).
    //   - Args... means the caller's argument is copied/moved once into the
    //     parameter pack at the call site, then forwarded into invoke().
    //     For move-only types (e.g. unique_ptr) the caller passes
    //     std::move(x) explicitly, which move-constructs into args.
    //   This matches std::function's calling convention exactly.
    R operator()(Args... args) const
    {
        if (!m_vtable)
        {
            // Calling an empty light_function is a hard programming error.
            abort();
        }
        // const_cast: the stored callable may have mutable state (e.g. a
        // mutable lambda). This mirrors std::function behaviour.
        return m_vtable->invoke(
            const_cast<light_function*>(this)->active_storage(),
            std::forward<Args>(args)...
        );
    }

    explicit operator bool() const noexcept
    {
        return m_vtable != nullptr;
    }

    void swap(light_function& other) noexcept
    {
        light_function tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);
    }
};

// Non-member comparison with nullptr
template<typename R, typename... Args, std::size_t S>
bool operator==(const light_function<R(Args...), S>& f, std::nullptr_t) noexcept
{
    return !f;
}

template<typename R, typename... Args, std::size_t S>
bool operator==(std::nullptr_t, const light_function<R(Args...), S>& f) noexcept
{
    return !f;
}

template<typename R, typename... Args, std::size_t S>
bool operator!=(const light_function<R(Args...), S>& f, std::nullptr_t) noexcept
{
    return !!f;
}

template<typename R, typename... Args, std::size_t S>
bool operator!=(std::nullptr_t, const light_function<R(Args...), S>& f) noexcept
{
    return !!f;
}

} // namespace lightstd
