#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include "rundown_protection.h"

// -----------------------------------------------------------------------------

static constexpr UBaseType_t kTaskPriority = tskIDLE_PRIORITY + 1;
static constexpr uint32_t    kTaskStackSize = 4096;
static constexpr TickType_t  kTestTimeout = pdMS_TO_TICKS(3000);

static constexpr EventBits_t kWaiter1ReadyBit = BIT0;
static constexpr EventBits_t kWaiter2ReadyBit = BIT1;
static constexpr EventBits_t kDestroyerReadyBit = BIT2;
static constexpr EventBits_t kAllowWaitBit = BIT3;
static constexpr EventBits_t kWaiter1FinishedBit = BIT4;
static constexpr EventBits_t kWaiter2FinishedBit = BIT5;
static constexpr EventBits_t kDestroyerDoneBit = BIT6;
static constexpr EventBits_t kRaceStartBit = BIT7;
static constexpr EventBits_t kRaceWaiterDoneBit = BIT8;
static constexpr EventBits_t kRaceWorkerDoneBits = BIT9 | BIT10 | BIT11 | BIT12;

typedef struct RundownTestContext_s {
    RundownProtection_t rp;
    EventGroupHandle_t  sync;
    StaticEventGroup_t  syncStorage;
} RundownTestContext_t;

typedef struct RundownWaiterContext_s {
    RundownTestContext_t* test;
    EventBits_t           readyBit;
    EventBits_t           finishedBit;
    bool                  destroy;
} RundownWaiterContext_t;

typedef struct RundownRaceContext_s {
    RundownProtection_t rp;
    EventGroupHandle_t  sync;
    StaticEventGroup_t  syncStorage;
} RundownRaceContext_t;

typedef struct RundownRaceWorkerContext_s {
    RundownRaceContext_t* test;
    EventBits_t           doneBit;
} RundownRaceWorkerContext_t;

static void rundownWaiterTask(void* arg);
static void rundownRaceWaiterTask(void* arg);
static void rundownRaceWorkerTask(void* arg);

TEST_CASE("RundownProtection basic lifecycle", "acquire/release/wait transitions")
{
    RundownProtection_t rp = RUNDOWN_PROTECTION_INIT_STATIC;

    TEST_ASSERT_TRUE(rundownProtAcquire(&rp));
    rundownProtRelease(&rp);

    rundownProtWait(&rp);
    TEST_ASSERT_FALSE(rundownProtAcquire(&rp));
    TEST_ASSERT_EQUAL_UINT32(0, atomic_load_explicit(&rp.waiters, memory_order_acquire));

    rundownProtDestroy(&rp);
    TEST_ASSERT_NULL(rp.eg);
}

TEST_CASE("RundownProtection wait without references", "wait still marks rundown active")
{
    RundownProtection_t rp;

    rundownProtInit(&rp);
    rundownProtWait(&rp);

    TEST_ASSERT_FALSE(rundownProtAcquire(&rp));

    rundownProtDestroy(&rp);
}

TEST_CASE("RundownProtection C++ wrapper", "acquire/release lifecycle")
{
    RundownProtection rp;

    TEST_ASSERT_TRUE(rp.Acquire());
    rp.Release();

    {
        AutoRundownProtection guard(rp);

        TEST_ASSERT_TRUE(guard.acquired());
    }

    rp.Wait();
    TEST_ASSERT_FALSE(rp.Acquire());
}

TEST_CASE("RundownProtection destruction waits for concurrent waiters", "concurrent wait and destroy")
{
    RundownTestContext_t   ctx{};
    RundownWaiterContext_t waiter1 = {.test = &ctx, .readyBit = kWaiter1ReadyBit, .finishedBit = kWaiter1FinishedBit, .destroy = false};
    RundownWaiterContext_t waiter2 = {.test = &ctx, .readyBit = kWaiter2ReadyBit, .finishedBit = kWaiter2FinishedBit, .destroy = false};
    RundownWaiterContext_t destroyer = {.test = &ctx, .readyBit = kDestroyerReadyBit, .finishedBit = kDestroyerDoneBit, .destroy = true};

    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);
    rundownProtInit(&ctx.rp);
    TEST_ASSERT_TRUE(rundownProtAcquire(&ctx.rp));

    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(rundownWaiterTask, "rundown_waiter_1", kTaskStackSize, &waiter1, kTaskPriority, nullptr));
    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(rundownWaiterTask, "rundown_waiter_2", kTaskStackSize, &waiter2, kTaskPriority, nullptr));
    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(rundownWaiterTask, "rundown_destroyer", kTaskStackSize, &destroyer, kTaskPriority + 1, nullptr));

    EventBits_t bits =
        xEventGroupWaitBits(ctx.sync, kWaiter1ReadyBit | kWaiter2ReadyBit | kDestroyerReadyBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_EQUAL(kWaiter1ReadyBit | kWaiter2ReadyBit | kDestroyerReadyBit,
                      bits & (kWaiter1ReadyBit | kWaiter2ReadyBit | kDestroyerReadyBit));

    xEventGroupSetBits(ctx.sync, kAllowWaitBit);
    vTaskDelay(1);
    rundownProtRelease(&ctx.rp);

    bits = xEventGroupWaitBits(ctx.sync, kWaiter1FinishedBit | kWaiter2FinishedBit | kDestroyerDoneBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_EQUAL(kWaiter1FinishedBit | kWaiter2FinishedBit | kDestroyerDoneBit,
                      bits & (kWaiter1FinishedBit | kWaiter2FinishedBit | kDestroyerDoneBit));
    vEventGroupDelete(ctx.sync);
}

TEST_CASE("RundownProtection rejects racing acquirers after wait begins", "acquire and wait race")
{
    RundownRaceContext_t       ctx{};
    RundownRaceWorkerContext_t workers[4];

    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);
    rundownProtInit(&ctx.rp);
    TEST_ASSERT_TRUE(rundownProtAcquire(&ctx.rp));

    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(rundownRaceWaiterTask, "rundown_race_wait", kTaskStackSize, &ctx, kTaskPriority, nullptr));
    for (size_t i = 0; i < 4; i++)
    {
        workers[i] = {.test = &ctx, .doneBit = static_cast<EventBits_t>(BIT9) << i};
        TEST_ASSERT_EQUAL(pdPASS,
                          xTaskCreate(rundownRaceWorkerTask, "rundown_race_worker", kTaskStackSize, &workers[i], kTaskPriority, nullptr));
    }

    xEventGroupSetBits(ctx.sync, kRaceStartBit);
    rundownProtRelease(&ctx.rp);

    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kRaceWaiterDoneBit | kRaceWorkerDoneBits, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_EQUAL(kRaceWaiterDoneBit | kRaceWorkerDoneBits, bits & (kRaceWaiterDoneBit | kRaceWorkerDoneBits));
    TEST_ASSERT_FALSE(rundownProtAcquire(&ctx.rp));

    rundownProtDestroy(&ctx.rp);
    vEventGroupDelete(ctx.sync);
}

// -----------------------------------------------------------------------------

static void rundownWaiterTask(void* arg)
{
    RundownWaiterContext_t* waiter = static_cast<RundownWaiterContext_t*>(arg);

    xEventGroupSetBits(waiter->test->sync, waiter->readyBit);
    xEventGroupWaitBits(waiter->test->sync, kAllowWaitBit, pdFALSE, pdTRUE, portMAX_DELAY);
    rundownProtWait(&waiter->test->rp);
    if (waiter->destroy)
    {
        rundownProtDestroy(&waiter->test->rp);
    }
    xEventGroupSetBits(waiter->test->sync, waiter->finishedBit);
    vTaskDelete(nullptr);
}

static void rundownRaceWaiterTask(void* arg)
{
    RundownRaceContext_t* ctx = static_cast<RundownRaceContext_t*>(arg);

    xEventGroupWaitBits(ctx->sync, kRaceStartBit, pdFALSE, pdTRUE, portMAX_DELAY);
    rundownProtWait(&ctx->rp);
    xEventGroupSetBits(ctx->sync, kRaceWaiterDoneBit);
    vTaskDelete(nullptr);
}

static void rundownRaceWorkerTask(void* arg)
{
    RundownRaceWorkerContext_t* worker = static_cast<RundownRaceWorkerContext_t*>(arg);

    xEventGroupWaitBits(worker->test->sync, kRaceStartBit, pdFALSE, pdTRUE, portMAX_DELAY);
    while (rundownProtAcquire(&worker->test->rp))
    {
        rundownProtRelease(&worker->test->rp);
    }

    xEventGroupSetBits(worker->test->sync, worker->doneBit);
    vTaskDelete(nullptr);
}
