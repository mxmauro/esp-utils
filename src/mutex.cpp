#include "mutex.h"
#include <string.h>

// -----------------------------------------------------------------------------

void mutexInit(Mutex_t *mtx)
{
    assert(mtx);
    memset(mtx, 0, sizeof(Mutex_t));
    // Static FreeRTOS mutex creation uses caller-provided storage, so a valid
    // buffer should be sufficient and allocation failure is not expected here.
    mtx->h = xSemaphoreCreateMutexStatic(&mtx->staticBuffer);
}

void mutexDeinit(Mutex_t *mtx)
{
    assert(mtx);
    if (mtx->h) {
        vSemaphoreDelete(mtx->h);
        mtx->h = nullptr;
    }
}

void mutexLock(Mutex_t *mtx)
{
    assert(mtx);
    xSemaphoreTake(mtx->h, portMAX_DELAY);
}

void mutexUnlock(Mutex_t *mtx)
{
    assert(mtx);
    xSemaphoreGive(mtx->h);
}

void rwMutexInit(RwMutex_t *rwMtx)
{
    assert(rwMtx);
    memset(rwMtx, 0, sizeof(RwMutex_t));
    rwMtx->reader = xSemaphoreCreateMutexStatic(&rwMtx->readerStaticBuffer);
    rwMtx->writer = xSemaphoreCreateBinaryStatic(&rwMtx->writerStaticBuffer);
    assert(rwMtx->reader);
    assert(rwMtx->writer);
    xSemaphoreGive(rwMtx->writer);
}

void rwMutexDeinit(RwMutex_t *rwMtx)
{
    assert(rwMtx);
    if (rwMtx->reader) {
        vSemaphoreDelete(rwMtx->reader);
        rwMtx->reader = nullptr;
    }
    if (rwMtx->writer) {
        vSemaphoreDelete(rwMtx->writer);
        rwMtx->writer = nullptr;
    }
}

void rwMutexLockRead(RwMutex_t *rwMtx)
{
    assert(rwMtx);
    xSemaphoreTake(rwMtx->reader, portMAX_DELAY);
    rwMtx->readersCount += 1;
    if (rwMtx->readersCount == 1) {
        xSemaphoreTake(rwMtx->writer, portMAX_DELAY);
    }
    xSemaphoreGive(rwMtx->reader);
}

void rwMutexUnlockRead(RwMutex_t *rwMtx)
{
    assert(rwMtx);
    xSemaphoreTake(rwMtx->reader, portMAX_DELAY);
    rwMtx->readersCount -= 1;
    if (rwMtx->readersCount == 0) {
        xSemaphoreGive(rwMtx->writer);
    }
    xSemaphoreGive(rwMtx->reader);
}

void rwMutexLockWrite(RwMutex_t *rwMtx)
{
    assert(rwMtx);
    xSemaphoreTake(rwMtx->writer, portMAX_DELAY);
}

void rwMutexUnlockWrite(RwMutex_t *rwMtx)
{
    assert(rwMtx);
    xSemaphoreGive(rwMtx->writer);
}
