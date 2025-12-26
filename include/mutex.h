#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

// Mutex provides a simple mutex wrapper around FreeRTOS semaphores.
class Mutex
{
public:
    Mutex()
        {
            mtx = xSemaphoreCreateMutexStatic(&mtxStaticBuffer);
        };

    ~Mutex()
        {
            if (mtx) {
                vSemaphoreDelete(mtx);
                mtx = nullptr;
            }
        };

    void Lock()
        {
            xSemaphoreTake(mtx, portMAX_DELAY);
        };

    void Unlock()
        {
            xSemaphoreGive(mtx);
        };
private:
    SemaphoreHandle_t mtx;
    StaticSemaphore_t mtxStaticBuffer;
};

class RWMutex
{
public:
    RWMutex()
        {
            readerMtx = xSemaphoreCreateMutexStatic(&readerMtxStaticBuffer);
            writerMtx = xSemaphoreCreateMutexStatic(&writerMtxStaticBuffer);
            readersCount = 0;
        };

    ~RWMutex()
        {
            if (readerMtx) {
                vSemaphoreDelete(readerMtx);
                readerMtx = nullptr;
            }
            if (writerMtx) {
                vSemaphoreDelete(writerMtx);
                writerMtx = nullptr;
            }
        };

    void LockRead()
        {
            xSemaphoreTake(readerMtx, portMAX_DELAY);
            readersCount += 1;
            if (readersCount == 1) {
                xSemaphoreTake(writerMtx, portMAX_DELAY);  // First reader locks
            }
            xSemaphoreGive(readerMtx);
        };

    void UnlockRead()
        {
            xSemaphoreTake(readerMtx, portMAX_DELAY);
            readersCount -= 1;
            if (readersCount == 0) {
                xSemaphoreGive(writerMtx);  // Last reader unlocks
            }
            xSemaphoreGive(readerMtx);
        };

    void LockWrite()
        {
            xSemaphoreTake(writerMtx, portMAX_DELAY);
        };

    void UnlockWrite()
        {
            xSemaphoreGive(writerMtx);
        };

private:
    SemaphoreHandle_t writerMtx;
    StaticSemaphore_t writerMtxStaticBuffer;
    SemaphoreHandle_t readerMtx;
    StaticSemaphore_t readerMtxStaticBuffer;
    uint32_t readersCount;
};

class AutoMutex
{
public:
    AutoMutex(Mutex *_mtx)
        {
            mtx = _mtx;
            mtx->Lock();
        };

    ~AutoMutex()
        {
            mtx->Unlock();
        };

private:
    Mutex *mtx;
};


class AutoRWMutex
{
public:
    AutoRWMutex(RWMutex *_rwMtx, bool _shared)
        {
            rwMtx = _rwMtx;
            shared = _shared;
            if (shared) {
                rwMtx->LockRead();
            }
            else {
                rwMtx->LockWrite();
            }
        };

    ~AutoRWMutex()
        {
            if (shared) {
                rwMtx->UnlockRead();
            }
            else {
                rwMtx->UnlockWrite();
            }
        };

private:
    RWMutex *rwMtx;
    bool shared;
};
