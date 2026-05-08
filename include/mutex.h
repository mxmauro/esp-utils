#pragma once

// Mutex provides a simple mutex wrapper around FreeRTOS semaphores.
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// -----------------------------------------------------------------------------

typedef struct Mutex_s {
    SemaphoreHandle_t h;
    StaticSemaphore_t staticBuffer;
} Mutex_t;

// Stores the synchronization primitives for a readers-writer lock.
typedef struct RwMutex_s {
    SemaphoreHandle_t writer;
    StaticSemaphore_t writerStaticBuffer;
    SemaphoreHandle_t reader;
    StaticSemaphore_t readerStaticBuffer;
    uint32_t readersCount;
} RwMutex_t;

// -----------------------------------------------------------------------------

// Initializes a C mutex wrapper.
void mutexInit(Mutex_t *mtx);
// Releases any resources owned by a C mutex wrapper.
void mutexDeinit(Mutex_t *mtx);

// Acquires exclusive ownership of the mutex.
void mutexLock(Mutex_t *mtx);
// Releases exclusive ownership of the mutex.
void mutexUnlock(Mutex_t *mtx);

// -----------------------------------------------------------------------------

// Initializes a readers-writer mutex wrapper.
void rwMutexInit(RwMutex_t *rwMtx);
// Releases any resources owned by a readers-writer mutex wrapper.
void rwMutexDeinit(RwMutex_t *rwMtx);

// Acquires shared read access.
void rwMutexLockRead(RwMutex_t *rwMtx);
// Releases shared read access.
void rwMutexUnlockRead(RwMutex_t *rwMtx);
// Acquires exclusive write access.
void rwMutexLockWrite(RwMutex_t *rwMtx);
// Releases exclusive write access.
void rwMutexUnlockWrite(RwMutex_t *rwMtx);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

class Mutex
{
public:
    // Initializes the wrapped mutex.
    Mutex()
    {
        mutexInit(&mtx);
    }
    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;

    // Destroys the wrapped mutex.
    ~Mutex()
    {
        mutexDeinit(&mtx);
    }

    Mutex& operator=(const Mutex&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    // Acquires exclusive ownership of the mutex.
    void Lock()
    {
        mutexLock(&mtx);
    }

    // Releases exclusive ownership of the mutex.
    void Unlock()
    {
        mutexUnlock(&mtx);
    }

private:
    Mutex_t mtx;
};

class RWMutex
{
public:
    // Initializes the wrapped readers-writer mutex.
    RWMutex()
    {
        rwMutexInit(&rwMtx);
    }
    RWMutex(const RWMutex&) = delete;
    RWMutex(RWMutex&&) = delete;

    // Destroys the wrapped readers-writer mutex.
    ~RWMutex()
    {
        rwMutexDeinit(&rwMtx);
    }

    RWMutex& operator=(const RWMutex&) = delete;
    RWMutex& operator=(RWMutex&&) = delete;

    // Acquires shared read access.
    void LockRead()
    {
        rwMutexLockRead(&rwMtx);
    }

    // Releases shared read access.
    void UnlockRead()
    {
        rwMutexUnlockRead(&rwMtx);
    }

    // Acquires exclusive write access.
    void LockWrite()
    {
        rwMutexLockWrite(&rwMtx);
    }

    // Releases exclusive write access.
    void UnlockWrite()
    {
        rwMutexUnlockWrite(&rwMtx);
    }

private:
    RwMutex_t rwMtx;
};

class AutoMutex
{
public:
    // Locks the mutex for the lifetime of this guard.
    AutoMutex(Mutex &_mtx) : mtx(_mtx)
    {
        mtx.Lock();
    }
    AutoMutex(const AutoMutex&) = delete;
    AutoMutex(AutoMutex&&) = delete;

    // Unlocks the mutex when the guard goes out of scope.
    ~AutoMutex()
    {
        mtx.Unlock();
    }

    AutoMutex& operator=(const AutoMutex&) = delete;
    AutoMutex& operator=(AutoMutex&&) = delete;

private:
    Mutex &mtx;
};

class AutoRWMutex
{
public:
    // Locks the readers-writer mutex in shared or exclusive mode for this scope.
    AutoRWMutex(RWMutex &_rwMtx, bool _shared) : rwMtx(_rwMtx), shared(_shared)
    {
        if (shared) {
            rwMtx.LockRead();
        }
        else {
            rwMtx.LockWrite();
        }
    }
    AutoRWMutex(const AutoRWMutex&) = delete;
    AutoRWMutex(AutoRWMutex&&) = delete;

    // Unlocks the readers-writer mutex when the guard goes out of scope.
    ~AutoRWMutex()
    {
        if (shared) {
            rwMtx.UnlockRead();
        }
        else {
            rwMtx.UnlockWrite();
        }
    }

    AutoRWMutex& operator=(const AutoRWMutex&) = delete;
    AutoRWMutex& operator=(AutoRWMutex&&) = delete;

private:
    RWMutex &rwMtx;
    bool shared;
};

#ifdef __cplusplus
}
#endif // __cplusplus
