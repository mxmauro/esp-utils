#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include "mutex.h"

// -----------------------------------------------------------------------------

static constexpr UBaseType_t kTaskPriority = tskIDLE_PRIORITY + 1;
static constexpr uint32_t kTaskStackSize = 4096;
static constexpr TickType_t kTestTimeout = pdMS_TO_TICKS(3000);

static constexpr EventBits_t kReader1EnteredBit = BIT0;
static constexpr EventBits_t kReader2EnteredBit = BIT1;
static constexpr EventBits_t kWriterEnteredBit = BIT2;
static constexpr EventBits_t kReleaseReadersBit = BIT3;

// -----------------------------------------------------------------------------

typedef struct MutexTestContext_s {
    Mutex mutex;
    TaskHandle_t mainTask;
    int counter;
    int activeWorkers;
    int maxActiveWorkers;
} MutexTestContext_t;

typedef struct RwMutexTestContext_s {
    RWMutex rwMutex;
    Mutex statsMutex;
    EventGroupHandle_t sync;
    StaticEventGroup_t syncStorage;
    int readersInside;
    int maxReadersInside;
} RwMutexTestContext_t;

typedef struct RwReaderTaskArgs_s {
    RwMutexTestContext_t *ctx;
    EventBits_t enteredBit;
} RwReaderTaskArgs_t;

typedef struct RwWriterTaskArgs_s {
    RwMutexTestContext_t *ctx;
    EventBits_t enteredBit;
} RwWriterTaskArgs_t;

// -----------------------------------------------------------------------------

static void mutexWorkerTask(void *arg);
static void rwReaderTask(void *arg);
static void rwWriterTask(void *arg);

// -----------------------------------------------------------------------------

TEST_CASE("Mutex serializes concurrent task access", "Mutex")
{
    constexpr int kWorkerCount = 4;
    constexpr int kIterationsPerWorker = 8;

    MutexTestContext_t ctx{};
    ctx.mainTask = xTaskGetCurrentTaskHandle();

    for (int i = 0; i < kWorkerCount; ++i) {
        BaseType_t res = xTaskCreate(
            mutexWorkerTask,
            "mutex_worker",
            kTaskStackSize,
            &ctx,
            kTaskPriority,
            nullptr
        );
        TEST_ASSERT_EQUAL(pdPASS, res);
    }

    uint32_t completedWorkers = 0;
    while (completedWorkers < kWorkerCount) {
        uint32_t notified = ulTaskNotifyTake(pdTRUE, kTestTimeout);
        TEST_ASSERT_NOT_EQUAL_UINT32(0, notified);
        completedWorkers += notified;
    }

    TEST_ASSERT_EQUAL(kWorkerCount * kIterationsPerWorker, ctx.counter);
    TEST_ASSERT_EQUAL(1, ctx.maxActiveWorkers);
    TEST_ASSERT_EQUAL(0, ctx.activeWorkers);
}

TEST_CASE("RWMutex allows readers to overlap and blocks writers", "RWMutex")
{
    RwMutexTestContext_t ctx{};
    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);

    TEST_ASSERT_NOT_NULL(ctx.sync);

    RwReaderTaskArgs_t reader1{};
    reader1.ctx = &ctx;
    reader1.enteredBit = kReader1EnteredBit;

    RwReaderTaskArgs_t reader2{};
    reader2.ctx = &ctx;
    reader2.enteredBit = kReader2EnteredBit;

    RwWriterTaskArgs_t writer{};
    writer.ctx = &ctx;
    writer.enteredBit = kWriterEnteredBit;

    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(rwReaderTask, "rw_reader_1", kTaskStackSize, &reader1, kTaskPriority, nullptr));
    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(rwReaderTask, "rw_reader_2", kTaskStackSize, &reader2, kTaskPriority, nullptr));

    EventBits_t bits = xEventGroupWaitBits(
        ctx.sync,
        kReader1EnteredBit | kReader2EnteredBit,
        pdFALSE,
        pdTRUE,
        kTestTimeout
    );
    TEST_ASSERT_EQUAL(kReader1EnteredBit | kReader2EnteredBit, bits & (kReader1EnteredBit | kReader2EnteredBit));

    {
        AutoMutex statsLock(ctx.statsMutex);

        TEST_ASSERT_EQUAL(2, ctx.readersInside);
        TEST_ASSERT_EQUAL(2, ctx.maxReadersInside);
    }

    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(rwWriterTask, "rw_writer", kTaskStackSize, &writer, kTaskPriority, nullptr));
    vTaskDelay(pdMS_TO_TICKS(20));
    TEST_ASSERT_EQUAL(0, xEventGroupGetBits(ctx.sync) & kWriterEnteredBit);

    xEventGroupSetBits(ctx.sync, kReleaseReadersBit);

    bits = xEventGroupWaitBits(ctx.sync, kWriterEnteredBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kWriterEnteredBit);

    vTaskDelay(pdMS_TO_TICKS(20));
    {
        AutoMutex statsLock(ctx.statsMutex);

        TEST_ASSERT_EQUAL(0, ctx.readersInside);
    }

    vEventGroupDelete(ctx.sync);
}

// -----------------------------------------------------------------------------

static void mutexWorkerTask(void *arg)
{
    MutexTestContext_t *ctx = static_cast<MutexTestContext_t *>(arg);

    for (int i = 0; i < 8; ++i) {
        {
            AutoMutex lock(ctx->mutex);

            ctx->activeWorkers += 1;
            if (ctx->activeWorkers > ctx->maxActiveWorkers) {
                ctx->maxActiveWorkers = ctx->activeWorkers;
            }

            int current = ctx->counter;
            vTaskDelay(pdMS_TO_TICKS(1));
            ctx->counter = current + 1;
            ctx->activeWorkers -= 1;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    xTaskNotifyGive(ctx->mainTask);
    vTaskDelete(nullptr);
}

static void rwReaderTask(void *arg)
{
    RwReaderTaskArgs_t *taskArgs = static_cast<RwReaderTaskArgs_t *>(arg);
    RwMutexTestContext_t *ctx = taskArgs->ctx;

    {
        AutoRWMutex lock(ctx->rwMutex, true);

        {
            AutoMutex statsLock(ctx->statsMutex);

            ctx->readersInside += 1;
            if (ctx->readersInside > ctx->maxReadersInside) {
                ctx->maxReadersInside = ctx->readersInside;
            }
        }

        xEventGroupSetBits(ctx->sync, taskArgs->enteredBit);
        xEventGroupWaitBits(ctx->sync, kReleaseReadersBit, pdFALSE, pdTRUE, portMAX_DELAY);

        {
            AutoMutex statsLock(ctx->statsMutex);

            ctx->readersInside -= 1;
        }
    }

    vTaskDelete(nullptr);
}

static void rwWriterTask(void *arg)
{
    RwWriterTaskArgs_t *taskArgs = static_cast<RwWriterTaskArgs_t *>(arg);
    RwMutexTestContext_t *ctx = taskArgs->ctx;

    {
        AutoRWMutex lock(ctx->rwMutex, false);

        xEventGroupSetBits(ctx->sync, taskArgs->enteredBit);
    }

    vTaskDelete(nullptr);
}
