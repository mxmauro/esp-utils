#pragma once

// Mutex provides a simple mutex wrapper around FreeRTOS semaphores.
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// -----------------------------------------------------------------------------

typedef struct Mutex_s {
    SemaphoreHandle_t h;
    StaticSemaphore_t staticBuffer;
} Mutex_t;

typedef struct RwMutex_s {
    SemaphoreHandle_t writer;
    StaticSemaphore_t writerStaticBuffer;
    SemaphoreHandle_t reader;
    StaticSemaphore_t readerStaticBuffer;
    uint32_t readersCount;
} RwMutex_t;

// -----------------------------------------------------------------------------

void mutexInit(Mutex_t *mtx);
void mutexDeinit(Mutex_t *mtx);

void mutexLock(Mutex_t *mtx);
void mutexUnlock(Mutex_t *mtx);

// -----------------------------------------------------------------------------

void rwMutexInit(RwMutex_t *rwMtx);
void rwMutexDeinit(RwMutex_t *rwMtx);

void rwMutexLockRead(RwMutex_t *rwMtx);
void rwMutexUnlockRead(RwMutex_t *rwMtx);
void rwMutexLockWrite(RwMutex_t *rwMtx);
void rwMutexUnlockWrite(RwMutex_t *rwMtx);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

class Mutex
{
public:
    Mutex()
    {
        mutexInit(&mtx);
    }
    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;

    ~Mutex()
    {
        mutexDeinit(&mtx);
    }

    Mutex& operator=(const Mutex&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    void Lock()
    {
        mutexLock(&mtx);
    }

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
    RWMutex()
    {
        rwMutexInit(&rwMtx);
    }
    RWMutex(const RWMutex&) = delete;
    RWMutex(RWMutex&&) = delete;

    ~RWMutex()
    {
        rwMutexDeinit(&rwMtx);
    }

    RWMutex& operator=(const RWMutex&) = delete;
    RWMutex& operator=(RWMutex&&) = delete;

    void LockRead()
    {
        rwMutexLockRead(&rwMtx);
    }

    void UnlockRead()
    {
        rwMutexUnlockRead(&rwMtx);
    }

    void LockWrite()
    {
        rwMutexLockWrite(&rwMtx);
    }

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
    AutoMutex(Mutex &_mtx) : mtx(_mtx)
    {
        mtx.Lock();
    }
    AutoMutex(const AutoMutex&) = delete;
    AutoMutex(AutoMutex&&) = delete;

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
