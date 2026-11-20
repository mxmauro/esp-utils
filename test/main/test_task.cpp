#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdatomic.h>
#include "task.h"

// -----------------------------------------------------------------------------

// Marks callback parameters that are intentionally unused by this test.
#define UNREFERENCED_PARAMETER(parameter) static_cast<void>(parameter)

// -----------------------------------------------------------------------------

static constexpr UBaseType_t kTaskPriority = tskIDLE_PRIORITY + 1;
static constexpr uint32_t    kTaskStackSize = 4096;
static constexpr TickType_t  kTestTimeout = pdMS_TO_TICKS(3000);
static constexpr TickType_t  kSelfDetachWatchdogDelay = pdMS_TO_TICKS(100);
static constexpr uint32_t    kRaceIterations = 32;

static constexpr EventBits_t kCallbackStartedBit = BIT0;
static constexpr EventBits_t kAllowContinueBit = BIT1;
static constexpr EventBits_t kCreatorReturnedBit = BIT2;
static constexpr EventBits_t kCallbackReturnedBit = BIT3;
static constexpr EventBits_t kQueryStartedBit = BIT4;
static constexpr EventBits_t kQueryFinishedBit = BIT5;
static constexpr EventBits_t kSelfDetachedBit = BIT6;
static constexpr EventBits_t kRaceQuery1StartedBit = BIT7;
static constexpr EventBits_t kRaceQuery2StartedBit = BIT8;
static constexpr EventBits_t kRaceJoinStartedBit = BIT9;
static constexpr EventBits_t kRaceQuery1FinishedBit = BIT10;
static constexpr EventBits_t kRaceQuery2FinishedBit = BIT11;
static constexpr EventBits_t kSelfDetachWatchdogFiredBit = BIT12;
static constexpr EventBits_t kSelfDetachWatchdogDoneBit = BIT13;

// -----------------------------------------------------------------------------

typedef struct TaskTestContext_s {
    EventGroupHandle_t    sync;
    StaticEventGroup_t    syncStorage;
    Task_t                task;
    _Atomic(TaskHandle_t) worker;
    BaseType_t            core;
    int                   copiedValue;
} TaskTestContext_t;

typedef struct TaskStartupArgs_s {
    TaskTestContext_t* ctx;
    int                value;
} TaskStartupArgs_t;

typedef struct TaskRaceQueryContext_s {
    TaskTestContext_t* ctx;
    EventBits_t        startedBit;
    EventBits_t        finishedBit;
} TaskRaceQueryContext_t;

// -----------------------------------------------------------------------------

static TaskHandle_t sMainTask;

static void nullArgumentCallback(Task_t task, void* arg);
static void parameterCallback(Task_t task, void* arg);
static void returnWithoutSignalCallback(Task_t task, void* arg);
static void cooperativeQuitCallback(Task_t task, void* arg);
static void externalDetachCallback(Task_t task, void* arg);
static void selfDetachCallback(Task_t task, void* arg);
static void wrapperCallback(Task_t task, void* arg);
static void createParameterTask(void* arg);
static void createReturnWithoutSignalTask(void* arg);
static void queryTask(void* arg);
static void raceQueryTask(void* arg);
static void selfDetachCreatorTask(void* arg);
static void selfDetachPriorityCallback(Task_t task, void* arg);
static void selfDetachWatchdogTask(void* arg);

// -----------------------------------------------------------------------------

TEST_CASE("Task starts automatically without parameters", "task")
{
    Task_t task;

    sMainTask = xTaskGetCurrentTaskHandle();
    task = taskCreate(nullArgumentCallback, "task_null_arg", kTaskStackSize, nullptr, kTaskPriority, tskNO_AFFINITY);

    TEST_ASSERT_NOT_NULL(task);
    TEST_ASSERT_NOT_EQUAL_UINT32(0, ulTaskNotifyTake(pdTRUE, kTestTimeout));

    taskJoin(task);
    task = nullptr;
    TEST_ASSERT_NULL(task);
}

TEST_CASE("Task C++ wrapper explicitly joins its task", "task C++ wrapper")
{
    sMainTask = xTaskGetCurrentTaskHandle();
    Task task = Task::Create(wrapperCallback, "task_wrapper", kTaskStackSize, nullptr, kTaskPriority, tskNO_AFFINITY);

    TEST_ASSERT_TRUE(static_cast<bool>(task));
    TEST_ASSERT_NOT_EQUAL_UINT32(0, ulTaskNotifyTake(pdTRUE, kTestTimeout));

    task.Join();
    TEST_ASSERT_FALSE(static_cast<bool>(task));
}

TEST_CASE("Task waits for non-null startup parameters", "task")
{
    TaskTestContext_t ctx{};

    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);

    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(createParameterTask, "task_creator", kTaskStackSize, &ctx, kTaskPriority, nullptr));
    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kCallbackStartedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kCallbackStartedBit);
    TEST_ASSERT_EQUAL(0, xEventGroupGetBits(ctx.sync) & kCreatorReturnedBit);

    xEventGroupSetBits(ctx.sync, kAllowContinueBit);
    bits = xEventGroupWaitBits(ctx.sync, kCreatorReturnedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kCreatorReturnedBit);
    TEST_ASSERT_EQUAL(42, ctx.copiedValue);

    taskJoin(ctx.task);
    ctx.task = nullptr;
    vEventGroupDelete(ctx.sync);
}

TEST_CASE("Task creation unblocks when callback returns without signaling", "task")
{
    TaskTestContext_t ctx{};

    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);

    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(createReturnWithoutSignalTask, "task_creator", kTaskStackSize, &ctx, kTaskPriority, nullptr));
    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kCreatorReturnedBit | kCallbackReturnedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_EQUAL(kCreatorReturnedBit | kCallbackReturnedBit, bits & (kCreatorReturnedBit | kCallbackReturnedBit));
    TEST_ASSERT_NOT_NULL(ctx.task);

    taskJoin(ctx.task);
    ctx.task = nullptr;
    vEventGroupDelete(ctx.sync);
}

TEST_CASE("Task join requests cooperative quit and waits for active queries", "task")
{
    TaskTestContext_t ctx{};

    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);
    ctx.task = taskCreate(cooperativeQuitCallback, "task_join", kTaskStackSize, &ctx, kTaskPriority, tskNO_AFFINITY);
    TEST_ASSERT_NOT_NULL(ctx.task);

    TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(queryTask, "task_query", kTaskStackSize, &ctx, kTaskPriority, nullptr));
    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kQueryStartedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kQueryStartedBit);

    taskJoin(ctx.task);
    ctx.task = nullptr;
    bits = xEventGroupWaitBits(ctx.sync, kQueryFinishedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kQueryFinishedBit);

    vEventGroupDelete(ctx.sync);
}

TEST_CASE("Task can detach after a natural callback exit", "task")
{
    TaskTestContext_t ctx{};
    TickType_t        started;

    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);
    ctx.task = taskCreate(externalDetachCallback, "task_detach", kTaskStackSize, &ctx, kTaskPriority, tskNO_AFFINITY);
    TEST_ASSERT_NOT_NULL(ctx.task);

    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kCallbackReturnedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kCallbackReturnedBit);
    started = xTaskGetTickCount();
    while (taskIsRunning(ctx.task))
    {
        TEST_ASSERT_LESS_THAN_UINT32(kTestTimeout, xTaskGetTickCount() - started);
        taskYIELD();
    }

    taskDetach(ctx.task);
    ctx.task = nullptr;
    TEST_ASSERT_NULL(ctx.task);
    vEventGroupDelete(ctx.sync);
}

TEST_CASE("Task status permits external detach after callback exit under contention", "task")
{
    for (uint32_t iteration = 0; iteration < kRaceIterations; iteration++)
    {
        TaskTestContext_t ctx{};

        ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
        TEST_ASSERT_NOT_NULL(ctx.sync);
        ctx.task = taskCreate(externalDetachCallback, "task_detach_race", kTaskStackSize, &ctx, kTaskPriority, tskNO_AFFINITY);
        TEST_ASSERT_NOT_NULL(ctx.task);

        EventBits_t bits = xEventGroupWaitBits(ctx.sync, kCallbackReturnedBit, pdFALSE, pdTRUE, kTestTimeout);
        TEST_ASSERT_NOT_EQUAL(0, bits & kCallbackReturnedBit);
        while (taskIsRunning(ctx.task))
        {
            taskYIELD();
        }

        taskDetach(ctx.task);
        ctx.task = nullptr;
        vEventGroupDelete(ctx.sync);

        // FreeRTOS reclaims deleted task stacks from its idle task. Yield before
        // creating the next iteration so the stress test does not exhaust heap.
        vTaskDelay(1);
    }
}

TEST_CASE("Task join safely handles queries that begin concurrently", "task")
{
    for (uint32_t iteration = 0; iteration < kRaceIterations; iteration++)
    {
        TaskTestContext_t      ctx{};
        TaskRaceQueryContext_t query1;
        TaskRaceQueryContext_t query2;

        ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
        TEST_ASSERT_NOT_NULL(ctx.sync);
        ctx.task = taskCreate(cooperativeQuitCallback, "task_join_race", kTaskStackSize, &ctx, kTaskPriority, tskNO_AFFINITY);
        TEST_ASSERT_NOT_NULL(ctx.task);

        query1 = {.ctx = &ctx, .startedBit = kRaceQuery1StartedBit, .finishedBit = kRaceQuery1FinishedBit};
        query2 = {.ctx = &ctx, .startedBit = kRaceQuery2StartedBit, .finishedBit = kRaceQuery2FinishedBit};
        TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(raceQueryTask, "task_query_race_1", kTaskStackSize, &query1, kTaskPriority, nullptr));
        TEST_ASSERT_EQUAL(pdPASS, xTaskCreate(raceQueryTask, "task_query_race_2", kTaskStackSize, &query2, kTaskPriority, nullptr));

        EventBits_t bits = xEventGroupWaitBits(ctx.sync, kRaceQuery1StartedBit | kRaceQuery2StartedBit, pdFALSE, pdTRUE, kTestTimeout);
        TEST_ASSERT_EQUAL(kRaceQuery1StartedBit | kRaceQuery2StartedBit, bits & (kRaceQuery1StartedBit | kRaceQuery2StartedBit));

        xEventGroupSetBits(ctx.sync, kRaceJoinStartedBit);
        taskJoin(ctx.task);

        bits = xEventGroupWaitBits(ctx.sync, kRaceQuery1FinishedBit | kRaceQuery2FinishedBit, pdFALSE, pdTRUE, kTestTimeout);
        TEST_ASSERT_EQUAL(kRaceQuery1FinishedBit | kRaceQuery2FinishedBit, bits & (kRaceQuery1FinishedBit | kRaceQuery2FinishedBit));
        ctx.task = nullptr;
        vEventGroupDelete(ctx.sync);

        // Let the idle task reclaim the worker and query task stacks before the
        // next stress-test iteration.
        vTaskDelay(1);
    }
}

TEST_CASE("Task self-detach does not starve a lower priority creator", "task")
{
    TaskTestContext_t ctx{};

    if (portNUM_PROCESSORS < 2)
    {
        TEST_IGNORE_MESSAGE("requires a multicore target");
    }

    ctx.core = xPortGetCoreID();
    atomic_init(&ctx.worker, nullptr);
    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);
    TEST_ASSERT_EQUAL(pdPASS, xTaskCreatePinnedToCore(selfDetachCreatorTask, "task_self_creator", kTaskStackSize, &ctx, kTaskPriority,
                                                      nullptr, ctx.core));
    TEST_ASSERT_EQUAL(pdPASS, xTaskCreatePinnedToCore(selfDetachWatchdogTask, "task_self_watchdog", kTaskStackSize, &ctx, kTaskPriority,
                                                      nullptr, (ctx.core + 1) % portNUM_PROCESSORS));

    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kCreatorReturnedBit | kSelfDetachWatchdogDoneBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_EQUAL(kCreatorReturnedBit | kSelfDetachWatchdogDoneBit, bits & (kCreatorReturnedBit | kSelfDetachWatchdogDoneBit));
    TEST_ASSERT_EQUAL(0, bits & kSelfDetachWatchdogFiredBit);

    bits = xEventGroupWaitBits(ctx.sync, kSelfDetachedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kSelfDetachedBit);
    vEventGroupDelete(ctx.sync);
}

TEST_CASE("Task can detach itself before deleting its FreeRTOS task", "task")
{
    TaskTestContext_t ctx{};

    ctx.sync = xEventGroupCreateStatic(&ctx.syncStorage);
    TEST_ASSERT_NOT_NULL(ctx.sync);
    ctx.task = taskCreate(selfDetachCallback, "task_self_detach", kTaskStackSize, &ctx, kTaskPriority, tskNO_AFFINITY);
    TEST_ASSERT_NOT_NULL(ctx.task);
    ctx.task = nullptr;

    EventBits_t bits = xEventGroupWaitBits(ctx.sync, kSelfDetachedBit, pdFALSE, pdTRUE, kTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, bits & kSelfDetachedBit);
    TEST_ASSERT_NULL(ctx.task);
    vTaskDelay(1);
    vEventGroupDelete(ctx.sync);
}

// -----------------------------------------------------------------------------

static void nullArgumentCallback(Task_t task, void* arg)
{
    UNREFERENCED_PARAMETER(task);
    UNREFERENCED_PARAMETER(arg);
    xTaskNotifyGive(sMainTask);
}

static void wrapperCallback(Task_t task, void* arg)
{
    UNREFERENCED_PARAMETER(task);
    UNREFERENCED_PARAMETER(arg);
    xTaskNotifyGive(sMainTask);
}

static void parameterCallback(Task_t task, void* arg)
{
    TaskStartupArgs_t* args = static_cast<TaskStartupArgs_t*>(arg);
    TaskTestContext_t* ctx = args->ctx;

    ctx->copiedValue = args->value;
    xEventGroupSetBits(ctx->sync, kCallbackStartedBit);
    xEventGroupWaitBits(ctx->sync, kAllowContinueBit, pdFALSE, pdTRUE, portMAX_DELAY);
    taskSignalContinue(task);
}

static void returnWithoutSignalCallback(Task_t task, void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    UNREFERENCED_PARAMETER(task);
    xEventGroupSetBits(ctx->sync, kCallbackReturnedBit);
}

static void cooperativeQuitCallback(Task_t task, void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    taskSignalContinue(task);
    while (!taskShouldQuit(task))
    {
        taskYIELD();
    }

    UNREFERENCED_PARAMETER(ctx);
}

static void externalDetachCallback(Task_t task, void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    taskSignalContinue(task);
    xEventGroupSetBits(ctx->sync, kCallbackReturnedBit);
}

static void selfDetachCallback(Task_t task, void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    taskSignalContinue(task);
    taskDetach(task);
    xEventGroupSetBits(ctx->sync, kSelfDetachedBit);
    vTaskDelete(nullptr);
}

static void createParameterTask(void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);
    TaskStartupArgs_t  startupArgs = {.ctx = ctx, .value = 42};

    ctx->task = taskCreate(parameterCallback, "task_parameter", kTaskStackSize, &startupArgs, kTaskPriority, tskNO_AFFINITY);
    xEventGroupSetBits(ctx->sync, kCreatorReturnedBit);
    vTaskDelete(nullptr);
}

static void createReturnWithoutSignalTask(void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    ctx->task = taskCreate(returnWithoutSignalCallback, "task_auto_continue", kTaskStackSize, ctx, kTaskPriority, tskNO_AFFINITY);
    xEventGroupSetBits(ctx->sync, kCreatorReturnedBit);
    vTaskDelete(nullptr);
}

static void queryTask(void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    xEventGroupSetBits(ctx->sync, kQueryStartedBit);
    while (taskIsRunning(ctx->task))
    {
        taskYIELD();
    }
    xEventGroupSetBits(ctx->sync, kQueryFinishedBit);
    vTaskDelete(nullptr);
}

static void raceQueryTask(void* arg)
{
    TaskRaceQueryContext_t* query = static_cast<TaskRaceQueryContext_t*>(arg);

    xEventGroupSetBits(query->ctx->sync, query->startedBit);
    xEventGroupWaitBits(query->ctx->sync, kRaceJoinStartedBit, pdFALSE, pdTRUE, portMAX_DELAY);
    while (taskIsRunning(query->ctx->task))
    {
        taskYIELD();
    }
    xEventGroupSetBits(query->ctx->sync, query->finishedBit);
    vTaskDelete(nullptr);
}

static void selfDetachCreatorTask(void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);
    Task_t task = taskCreate(selfDetachPriorityCallback, "task_self_priority", kTaskStackSize, ctx, kTaskPriority + 1, ctx->core);

    UNREFERENCED_PARAMETER(task);
    xEventGroupSetBits(ctx->sync, kCreatorReturnedBit);
    vTaskDelete(nullptr);
}

static void selfDetachPriorityCallback(Task_t task, void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    atomic_store_explicit(&ctx->worker, xTaskGetCurrentTaskHandle(), memory_order_release);
    xEventGroupSetBits(ctx->sync, kCallbackStartedBit);
    taskDetach(task);
    xEventGroupSetBits(ctx->sync, kSelfDetachedBit);
    vTaskDelete(nullptr);
}

static void selfDetachWatchdogTask(void* arg)
{
    TaskTestContext_t* ctx = static_cast<TaskTestContext_t*>(arg);

    xEventGroupWaitBits(ctx->sync, kCallbackStartedBit, pdFALSE, pdTRUE, portMAX_DELAY);
    vTaskDelay(kSelfDetachWatchdogDelay);
    if ((xEventGroupGetBits(ctx->sync) & kCreatorReturnedBit) == 0)
    {
        vTaskDelete(atomic_load_explicit(&ctx->worker, memory_order_acquire));
        xEventGroupSetBits(ctx->sync, kSelfDetachWatchdogFiredBit);
    }
    xEventGroupSetBits(ctx->sync, kSelfDetachWatchdogDoneBit);
    vTaskDelete(nullptr);
}
