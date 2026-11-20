#pragma once

// Mutex provides a simple mutex wrapper around FreeRTOS semaphores.
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdint.h>

// -----------------------------------------------------------------------------

typedef struct Mutex_s {
    SemaphoreHandle_t h;
    StaticSemaphore_t staticBuffer;
} Mutex_t;

// Stores the synchronization primitives for a readers-writer lock.
typedef struct RwMutex_s {
    SemaphoreHandle_t readerGate;
    StaticSemaphore_t readerGateStaticBuffer;
    SemaphoreHandle_t writerCountMutex;
    StaticSemaphore_t writerCountMutexStaticBuffer;
    uint32_t          writersCount;
    SemaphoreHandle_t writer;
    StaticSemaphore_t writerStaticBuffer;
    SemaphoreHandle_t reader;
    StaticSemaphore_t readerStaticBuffer;
    uint32_t          readersCount;
} RwMutex_t;

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes a C mutex wrapper.
void mutexInit(Mutex_t* mtx);
// Releases any resources owned by a C mutex wrapper.
void mutexDeinit(Mutex_t* mtx);

// Acquires exclusive ownership of the mutex.
void mutexLock(Mutex_t* mtx);
// Releases exclusive ownership of the mutex.
void mutexUnlock(Mutex_t* mtx);

// -----------------------------------------------------------------------------

// Initializes a readers-writer mutex wrapper.
void rwMutexInit(RwMutex_t* rwMtx);
// Releases any resources owned by a readers-writer mutex wrapper.
void rwMutexDeinit(RwMutex_t* rwMtx);

// Acquires shared read access. Waiting writers are given priority over new readers.
void rwMutexLockRead(RwMutex_t* rwMtx);
// Releases shared read access.
void rwMutexUnlockRead(RwMutex_t* rwMtx);
// Acquires exclusive write access. Waiting writers block new readers.
void rwMutexLockWrite(RwMutex_t* rwMtx);
// Releases exclusive write access.
void rwMutexUnlockWrite(RwMutex_t* rwMtx);

#ifdef __cplusplus
}
#endif // __cplusplus

// -----------------------------------------------------------------------------

#ifdef __cplusplus

class Mutex
{
public:
    // Initializes the wrapped mutex.
    Mutex() noexcept
    {
        mutexInit(&mtx);
    }
    Mutex(const Mutex&) noexcept = delete;
    Mutex(Mutex&&) noexcept = delete;

    // Destroys the wrapped mutex.
    ~Mutex() noexcept
    {
        mutexDeinit(&mtx);
    }

    Mutex& operator=(const Mutex&) noexcept = delete;
    Mutex& operator=(Mutex&&) noexcept = delete;

    // Acquires exclusive ownership of the mutex.
    void Lock() noexcept
    {
        mutexLock(&mtx);
    }

    // Releases exclusive ownership of the mutex.
    void Unlock() noexcept
    {
        mutexUnlock(&mtx);
    }

private:
    Mutex_t mtx;
};

// -----------------------------------------------------------------------------

class RWMutex
{
public:
    // Initializes the wrapped readers-writer mutex.
    RWMutex() noexcept
    {
        rwMutexInit(&rwMtx);
    }
    RWMutex(const RWMutex&) noexcept = delete;
    RWMutex(RWMutex&&) noexcept = delete;

    // Destroys the wrapped readers-writer mutex.
    ~RWMutex() noexcept
    {
        rwMutexDeinit(&rwMtx);
    }

    RWMutex& operator=(const RWMutex&) noexcept = delete;
    RWMutex& operator=(RWMutex&&) noexcept = delete;

    // Acquires shared read access.
    void LockRead() noexcept
    {
        rwMutexLockRead(&rwMtx);
    }

    // Releases shared read access.
    void UnlockRead() noexcept
    {
        rwMutexUnlockRead(&rwMtx);
    }

    // Acquires exclusive write access.
    void LockWrite() noexcept
    {
        rwMutexLockWrite(&rwMtx);
    }

    // Releases exclusive write access.
    void UnlockWrite() noexcept
    {
        rwMutexUnlockWrite(&rwMtx);
    }

private:
    RwMutex_t rwMtx;
};

// -----------------------------------------------------------------------------

class AutoMutex
{
public:
    // Locks the mutex for the lifetime of this guard.
    AutoMutex(Mutex& _mtx) noexcept : mtx(_mtx)
    {
        mtx.Lock();
    }
    AutoMutex(const AutoMutex&) noexcept = delete;
    AutoMutex(AutoMutex&&) noexcept = delete;

    // Unlocks the mutex when the guard goes out of scope.
    ~AutoMutex() noexcept
    {
        mtx.Unlock();
    }

    AutoMutex& operator=(const AutoMutex&) noexcept = delete;
    AutoMutex& operator=(AutoMutex&&) noexcept = delete;

private:
    Mutex& mtx;
};

// -----------------------------------------------------------------------------

class AutoRWMutex
{
public:
    // Locks the readers-writer mutex in shared or exclusive mode for this scope.
    AutoRWMutex(RWMutex& _rwMtx, bool _shared) noexcept : rwMtx(_rwMtx), shared(_shared)
    {
        if (shared)
        {
            rwMtx.LockRead();
        }
        else
        {
            rwMtx.LockWrite();
        }
    }
    AutoRWMutex(const AutoRWMutex&) noexcept = delete;
    AutoRWMutex(AutoRWMutex&&) noexcept = delete;

    // Unlocks the readers-writer mutex when the guard goes out of scope.
    ~AutoRWMutex() noexcept
    {
        if (shared)
        {
            rwMtx.UnlockRead();
        }
        else
        {
            rwMtx.UnlockWrite();
        }
    }

    AutoRWMutex& operator=(const AutoRWMutex&) noexcept = delete;
    AutoRWMutex& operator=(AutoRWMutex&&) noexcept = delete;

private:
    RWMutex& rwMtx;
    bool     shared;
};

#endif // __cplusplus
