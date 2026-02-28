#pragma once

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// -----------------------------------------------------------------------------

// Mutex provides a simple mutex wrapper around FreeRTOS semaphores.
class Mutex
{
public:
    Mutex()
    {
        mtx = xSemaphoreCreateMutexStatic(&mtxStaticBuffer);
    }
    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;

    ~Mutex()
    {
        vSemaphoreDelete(mtx);
    }

    Mutex& operator=(const Mutex&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    void Lock()
    {
        xSemaphoreTake(mtx, portMAX_DELAY);
    }

    void Unlock()
    {
        xSemaphoreGive(mtx);
    }

private:
    SemaphoreHandle_t mtx{nullptr};
    StaticSemaphore_t mtxStaticBuffer{};
};

class RWMutex
{
public:
    RWMutex()
    {
        readerMtx = xSemaphoreCreateMutexStatic(&readerMtxStaticBuffer);
        writerMtx = xSemaphoreCreateMutexStatic(&writerMtxStaticBuffer);
        readersCount = 0;
    }
    RWMutex(const RWMutex&) = delete;
    RWMutex(RWMutex&&) = delete;

    ~RWMutex()
    {
        vSemaphoreDelete(readerMtx);
        vSemaphoreDelete(writerMtx);
    }

    RWMutex& operator=(const RWMutex&) = delete;
    RWMutex& operator=(RWMutex&&) = delete;

    void LockRead()
    {
        xSemaphoreTake(readerMtx, portMAX_DELAY);
        readersCount += 1;
        if (readersCount == 1) {
            xSemaphoreTake(writerMtx, portMAX_DELAY);  // First reader locks
        }
        xSemaphoreGive(readerMtx);
    }

    void UnlockRead()
    {
        xSemaphoreTake(readerMtx, portMAX_DELAY);
        readersCount -= 1;
        if (readersCount == 0) {
            xSemaphoreGive(writerMtx);  // Last reader unlocks
        }
        xSemaphoreGive(readerMtx);
    }

    void LockWrite()
    {
        xSemaphoreTake(writerMtx, portMAX_DELAY);
    }

    void UnlockWrite()
    {
        xSemaphoreGive(writerMtx);
    }

private:
    SemaphoreHandle_t writerMtx{nullptr};
    StaticSemaphore_t writerMtxStaticBuffer{};
    SemaphoreHandle_t readerMtx{nullptr};
    StaticSemaphore_t readerMtxStaticBuffer{};
    uint32_t readersCount{0};
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
