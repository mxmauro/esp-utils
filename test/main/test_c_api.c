#include <unity.h>
#include "convert.h"
#include "fnv.h"
#include "growable_buffer.h"
#include "mutex.h"
#include "onboard_led.h"
#include "reset_button.h"
#include "rundown_protection.h"
#include "run_once.h"
#include "storage/nvs.h"
#include "task.h"
#include "time.h"

// -----------------------------------------------------------------------------

static void cApiRunOnceCallback(void* arg)
{
    int* count = arg;

    *count += 1;
}

static void cApiTaskCallback(Task_t task, void* arg)
{
    int* ran = arg;

    *ran = 1;
    taskSignalContinue(task);
}

// -----------------------------------------------------------------------------

TEST_CASE("C-facing APIs compile and link from C", "C API")
{
    GrowableBuffer_t    buffer = GB_STATIC_INIT;
    Mutex_t             mutex;
    RwMutex_t           rwMutex;
    RunOnce_t           once = RUN_ONCE_INIT_STATIC;
    RundownProtection_t rundown = RUNDOWN_PROTECTION_INIT_STATIC;
    Task_t              task;
    size_t              hexLen = 3;
    char                hex[3];
    int                 onceCount = 0;
    int                 taskRan = 0;
    esp_err_t (*nvsInitializer)(void) = nvsInit;
    uint64_t (*clockNow)(void) = now_ms;
    void (*onboardInitializer)(gpio_num_t) = onboardLedInit;
    void (*onboardSetter)(bool) = onboardLedSet;
    void (*resetButtonInitializer)(gpio_num_t, ResetButtonPressedHandler_t, void*) = setupResetButton;

    TEST_ASSERT_TRUE(gbAdd(&buffer, "x", 1, SIZE_MAX));
    TEST_ASSERT_EQUAL_UINT32(1, buffer.used);
    gbReset(&buffer, true);

    TEST_ASSERT_TRUE(toHex("x", 1, hex, &hexLen));
    TEST_ASSERT_EQUAL_UINT32(2, hexLen);
    TEST_ASSERT_EQUAL_UINT32(FNV1A32_INITIAL_HASH, fnv1a32(NULL, 0, FNV1A32_INITIAL_HASH));

    mutexInit(&mutex);
    mutexLock(&mutex);
    mutexUnlock(&mutex);
    mutexDeinit(&mutex);

    rwMutexInit(&rwMutex);
    rwMutexLockRead(&rwMutex);
    rwMutexUnlockRead(&rwMutex);
    rwMutexLockWrite(&rwMutex);
    rwMutexUnlockWrite(&rwMutex);
    rwMutexDeinit(&rwMutex);

    runOnce(&once, cApiRunOnceCallback, &onceCount);
    runOnce(&once, cApiRunOnceCallback, &onceCount);
    TEST_ASSERT_EQUAL(1, onceCount);

    rundownProtInit(&rundown);
    TEST_ASSERT_TRUE(rundownProtAcquire(&rundown));
    rundownProtRelease(&rundown);
    rundownProtWait(&rundown);
    rundownProtDestroy(&rundown);

    task = taskCreate(cApiTaskCallback, "c_api_task", 4096, &taskRan, tskIDLE_PRIORITY + 1, tskNO_AFFINITY);
    TEST_ASSERT_NOT_NULL(task);
    taskJoin(task);
    TEST_ASSERT_EQUAL(1, taskRan);

    TEST_ASSERT_NOT_NULL(nvsInitializer);
    TEST_ASSERT_NOT_NULL(onboardInitializer);
    TEST_ASSERT_NOT_NULL(onboardSetter);
    TEST_ASSERT_NOT_NULL(resetButtonInitializer);
    TEST_ASSERT_NOT_NULL(clockNow);
}
