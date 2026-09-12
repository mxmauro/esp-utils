#include "mutex.h"
#include <assert.h>
#include <string.h>

// -----------------------------------------------------------------------------

void mutexInit(Mutex_t* mtx)
{
    assert(mtx);
    memset(mtx, 0, sizeof(Mutex_t));
    // Static FreeRTOS mutex creation uses caller-provided storage, so a valid
    // buffer should be sufficient and allocation failure is not expected here.
    mtx->h = xSemaphoreCreateMutexStatic(&mtx->staticBuffer);
    assert(mtx->h);
}

void mutexDeinit(Mutex_t* mtx)
{
    assert(mtx);
    assert(mtx->h);
    assert(uxSemaphoreGetCount(mtx->h) == 1);
    if (mtx->h)
    {
        vSemaphoreDelete(mtx->h);
        mtx->h = nullptr;
    }
}

void mutexLock(Mutex_t* mtx)
{
    assert(mtx);
    assert(mtx->h);
    assert(xSemaphoreTake(mtx->h, portMAX_DELAY) == pdPASS);
}

void mutexUnlock(Mutex_t* mtx)
{
    assert(mtx);
    assert(mtx->h);
    assert(xSemaphoreGive(mtx->h) == pdPASS);
}

void rwMutexInit(RwMutex_t* rwMtx)
{
    assert(rwMtx);
    memset(rwMtx, 0, sizeof(RwMutex_t));
    rwMtx->readerGate = xSemaphoreCreateBinaryStatic(&rwMtx->readerGateStaticBuffer);
    rwMtx->writerCountMutex = xSemaphoreCreateMutexStatic(&rwMtx->writerCountMutexStaticBuffer);
    rwMtx->reader = xSemaphoreCreateMutexStatic(&rwMtx->readerStaticBuffer);
    rwMtx->writer = xSemaphoreCreateBinaryStatic(&rwMtx->writerStaticBuffer);
    assert(rwMtx->readerGate);
    assert(rwMtx->writerCountMutex);
    assert(rwMtx->reader);
    assert(rwMtx->writer);
    assert(xSemaphoreGive(rwMtx->readerGate) == pdPASS);
    assert(xSemaphoreGive(rwMtx->writer) == pdPASS);
}

void rwMutexDeinit(RwMutex_t* rwMtx)
{
    assert(rwMtx);
    assert(rwMtx->readerGate);
    assert(rwMtx->writerCountMutex);
    assert(rwMtx->reader);
    assert(rwMtx->writer);
    assert(rwMtx->readersCount == 0);
    assert(rwMtx->writersCount == 0);
    assert(uxSemaphoreGetCount(rwMtx->readerGate) == 1);
    assert(uxSemaphoreGetCount(rwMtx->writerCountMutex) == 1);
    assert(uxSemaphoreGetCount(rwMtx->reader) == 1);
    assert(uxSemaphoreGetCount(rwMtx->writer) == 1);
    if (rwMtx->readerGate)
    {
        vSemaphoreDelete(rwMtx->readerGate);
        rwMtx->readerGate = nullptr;
    }
    if (rwMtx->writerCountMutex)
    {
        vSemaphoreDelete(rwMtx->writerCountMutex);
        rwMtx->writerCountMutex = nullptr;
    }
    if (rwMtx->reader)
    {
        vSemaphoreDelete(rwMtx->reader);
        rwMtx->reader = nullptr;
    }
    if (rwMtx->writer)
    {
        vSemaphoreDelete(rwMtx->writer);
        rwMtx->writer = nullptr;
    }
}

void rwMutexLockRead(RwMutex_t* rwMtx)
{
    assert(rwMtx);
    assert(rwMtx->readerGate);
    assert(rwMtx->reader);
    assert(rwMtx->writer);

    // The first waiting writer closes this gate until all waiting writers leave.
    assert(xSemaphoreTake(rwMtx->readerGate, portMAX_DELAY) == pdPASS);
    assert(xSemaphoreGive(rwMtx->readerGate) == pdPASS);

    assert(xSemaphoreTake(rwMtx->reader, portMAX_DELAY) == pdPASS);
    rwMtx->readersCount += 1;
    if (rwMtx->readersCount == 1)
    {
        assert(xSemaphoreTake(rwMtx->writer, portMAX_DELAY) == pdPASS);
    }
    assert(xSemaphoreGive(rwMtx->reader) == pdPASS);
}

void rwMutexUnlockRead(RwMutex_t* rwMtx)
{
    assert(rwMtx);
    assert(rwMtx->reader);
    assert(rwMtx->writer);
    assert(xSemaphoreTake(rwMtx->reader, portMAX_DELAY) == pdPASS);
    assert(rwMtx->readersCount > 0);
    rwMtx->readersCount -= 1;
    if (rwMtx->readersCount == 0)
    {
        assert(xSemaphoreGive(rwMtx->writer) == pdPASS);
    }
    assert(xSemaphoreGive(rwMtx->reader) == pdPASS);
}

void rwMutexLockWrite(RwMutex_t* rwMtx)
{
    assert(rwMtx);
    assert(rwMtx->readerGate);
    assert(rwMtx->writerCountMutex);
    assert(rwMtx->writer);

    assert(xSemaphoreTake(rwMtx->writerCountMutex, portMAX_DELAY) == pdPASS);
    rwMtx->writersCount += 1;
    if (rwMtx->writersCount == 1)
    {
        assert(xSemaphoreTake(rwMtx->readerGate, portMAX_DELAY) == pdPASS);
    }
    assert(xSemaphoreGive(rwMtx->writerCountMutex) == pdPASS);

    assert(xSemaphoreTake(rwMtx->writer, portMAX_DELAY) == pdPASS);
}

void rwMutexUnlockWrite(RwMutex_t* rwMtx)
{
    assert(rwMtx);
    assert(rwMtx->readerGate);
    assert(rwMtx->writerCountMutex);
    assert(rwMtx->writer);
    assert(xSemaphoreGive(rwMtx->writer) == pdPASS);

    assert(xSemaphoreTake(rwMtx->writerCountMutex, portMAX_DELAY) == pdPASS);
    assert(rwMtx->writersCount > 0);
    rwMtx->writersCount -= 1;
    if (rwMtx->writersCount == 0)
    {
        assert(xSemaphoreGive(rwMtx->readerGate) == pdPASS);
    }
    assert(xSemaphoreGive(rwMtx->writerCountMutex) == pdPASS);
}
