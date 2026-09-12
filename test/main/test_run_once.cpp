#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdatomic.h>
#include "run_once.h"

// -----------------------------------------------------------------------------

static constexpr UBaseType_t kTaskPriority = tskIDLE_PRIORITY + 1;
static constexpr uint32_t    kTaskStackSize = 4096;
static constexpr TickType_t  kTestTimeout = pdMS_TO_TICKS(3000);

static constexpr EventBits_t kStartBit = BIT0;
static constexpr EventBits_t kCallbackEnteredBit = BIT1;
static constexpr EventBits_t kReleaseCallbackBit = BIT2;
static constexpr EventBits_t kWorkerDoneBits = BIT3 | BIT4 | BIT5 | BIT6;

typedef struct RunOnceRaceContext_s {
    RunOnce_t          once;
    EventGroupHandle_t sync;
    StaticEventGroup_t syncStorage;
    _Atomic(uint32_t)  callbackCount;
} RunOnceRaceContext_t;

typedef struct RunOnceWorkerContext_s {
    RunOnceRaceContext_t* test;
    EventBits_t           doneBit;
} RunOnceWorkerContext_t;

static void runOnceRaceCallback(void* arg);
static void runOnceWorkerTask(void* arg);

// -----------------------------------------------------------------------------

TEST_CASE("RunOnce", "Expected behavior")
{
    RunOnce_t once;
    int       counter = 0;

    runOnceInit(&once);

    auto cb = [](void* arg)
    {
        int* counter = static_cast<int*>(arg);
        (*counter)++;
    };

    runOnce(&once, cb, &counter);
    runOnce(&once, cb, &counter);
    runOnce(&once, cb, &counter);

    TEST_ASSERT_EQUAL(1, counter);
}

TEST_CASE("RunOnce executes once under task contention", "concurrent one-time initialization")
{
    RunOnceRaceContext_t   ctx{};
    RunOnceWorkerContext_t workers[4];

    runOnceInit(&ctx.once);
    atomic_init(&ctx.callbackCount, 0);
    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);

    for (size_t i = 0; i < 4; i++)
    {
        workers[i] = {.test = &ctx, .doneBit = static_cast<EventBits_t>(BIT3) << i};
        TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(runOnceWorkerTask, "run_once_worker", kTaskStackSize, &workers[i], kTaskPriority, nullptr));
    }

    xEventGroupSetBits(ctx.sync, kStartBit);
    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kCallbackEnteredBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kCallbackEnteredBit);

    xEventGroupSetBits(ctx.sync, kReleaseCallbackBit);
    bits = xEventGroupWaitBits(ctx.sync, kWorkerDoneBits, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_EQUAL(kWorkerDoneBits, bits & kWorkerDoneBits);
    TEST_ASSERT_EQUAL_UINT32(1, atomic_load_explicit(&ctx.callbackCount, memory_order_acquire));

    vEventGroupDelete(ctx.sync);
}

// -----------------------------------------------------------------------------

static void runOnceRaceCallback(void* arg)
{
    RunOnceRaceContext_t* ctx = static_cast<RunOnceRaceContext_t*>(arg);

    atomic_fetch_add_explicit(&ctx->callbackCount, 1, memory_order_relaxed);
    xEventGroupSetBits(ctx->sync, kCallbackEnteredBit);
    xEventGroupWaitBits(ctx->sync, kReleaseCallbackBit, pdFALSE, pdTRUE, portMAX_DELAY);
}

static void runOnceWorkerTask(void* arg)
{
    RunOnceWorkerContext_t* worker = static_cast<RunOnceWorkerContext_t*>(arg);

    xEventGroupWaitBits(worker->test->sync, kStartBit, pdFALSE, pdTRUE, portMAX_DELAY);
    runOnce(&worker->test->once, runOnceRaceCallback, worker->test);
    xEventGroupSetBits(worker->test->sync, worker->doneBit);
    vTaskDelete(nullptr);
}
