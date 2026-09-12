#include "task.h"
#include "mutex.h"
#include "rundown_protection.h"
#include <freertos/event_groups.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>

#define SIGNAL_BIT_CONTINUE      BIT0
#define SIGNAL_BIT_MUST_QUIT     BIT1
#define SIGNAL_BIT_QUIT          BIT2
#define SIGNAL_BIT_CREATOR_READY BIT3

// -----------------------------------------------------------------------------

typedef enum TaskLifecycle_e : uint8_t {
    Running,
    Exited,
    Joining,
    Detached,
} TaskLifecycle_t;

typedef struct TaskState_s {
    EventGroupHandle_t       eg;
    RundownProtection_t      rundown;
    _Atomic(TaskLifecycle_t) lifecycle;
    _Atomic(TaskHandle_t)    worker;
    TaskRoutine_t            fn;
    void*                    pvParameters;
    TaskState_s*             next;
} TaskState_t;

// -----------------------------------------------------------------------------

static Mutex        sTaskRegistryMtx;
static TaskState_t* sTaskRegistryHead = nullptr;

static void         taskRoutine(void* pvParameters);
static void         taskSetContinue(TaskState_t* task);
static void         taskDestroy(TaskState_t* task);
static void         taskRegister(TaskState_t* task);
static void         taskUnregister(TaskState_t* task);
static void         taskUnregisterLocked(TaskState_t* task);
static TaskState_t* taskFindRegistered(Task_t task);
static bool         taskTryAcquire(Task_t task, TaskState_t** state);
static bool         taskTryBeginJoin(Task_t task, TaskState_t** state);
static bool         taskTryBeginDetach(Task_t task, TaskState_t** state, bool* selfDetach);

// -----------------------------------------------------------------------------

Task_t taskCreate(TaskRoutine_t fn, const char* pcName, uint32_t usStackDepth, void* pvParameters, UBaseType_t uxPriority,
                  BaseType_t xCoreID)
{
    TaskState_t* task;
    TaskHandle_t worker;

    if ((!fn) || (!pcName) || (*pcName == 0) || (usStackDepth == 0))
    {
        return nullptr;
    }

    task = static_cast<TaskState_t*>(malloc(sizeof(TaskState_t)));
    if (!task)
    {
        return nullptr;
    }

    atomic_init(&task->lifecycle, TaskLifecycle_t::Running);
    atomic_init(&task->worker, nullptr);
    task->fn = fn;
    task->pvParameters = pvParameters;
    task->next = nullptr;
    task->eg = xEventGroupCreate();
    if (!task->eg)
    {
        free(task);
        return nullptr;
    }

    rundownProtInit(&task->rundown);
    if (!rundownProtAcquire(&task->rundown))
    {
        taskDestroy(task);
        return nullptr;
    }

    taskRegister(task);

    if (xTaskCreatePinnedToCore(&taskRoutine, pcName, usStackDepth, task, uxPriority, &worker, xCoreID) != pdPASS)
    {
        taskUnregister(task);
        rundownProtRelease(&task->rundown);
        taskDestroy(task);
        return nullptr;
    }

    atomic_store_explicit(&task->worker, worker, memory_order_release);
    xEventGroupWaitBits(task->eg, SIGNAL_BIT_CONTINUE, pdFALSE, pdTRUE, portMAX_DELAY);
    xEventGroupSetBits(task->eg, SIGNAL_BIT_CREATOR_READY);

    return task;
}

void taskSignalContinue(Task_t task)
{
    TaskState_t* state;

    if (!taskTryAcquire(task, &state))
    {
        return;
    }

    assert(atomic_load_explicit(&state->worker, memory_order_acquire) == xTaskGetCurrentTaskHandle());
    if (atomic_load_explicit(&state->worker, memory_order_acquire) != xTaskGetCurrentTaskHandle())
    {
        rundownProtRelease(&state->rundown);
        return;
    }

    taskSetContinue(state);
    rundownProtRelease(&state->rundown);
}

bool taskShouldQuit(Task_t task)
{
    TaskState_t* state;
    bool         shouldQuit;

    if (!taskTryAcquire(task, &state))
    {
        return true;
    }

    shouldQuit = (atomic_load_explicit(&state->lifecycle, memory_order_acquire) != TaskLifecycle_t::Running) ||
                 ((xEventGroupGetBits(state->eg) & SIGNAL_BIT_MUST_QUIT) != 0);

    rundownProtRelease(&state->rundown);
    return shouldQuit;
}

bool taskIsRunning(Task_t task)
{
    TaskState_t* state;
    bool         isRunning;

    if (!taskTryAcquire(task, &state))
    {
        return false;
    }

    isRunning = (atomic_load_explicit(&state->lifecycle, memory_order_acquire) == TaskLifecycle_t::Running) &&
                ((xEventGroupGetBits(state->eg) & SIGNAL_BIT_QUIT) == 0);

    rundownProtRelease(&state->rundown);
    return isRunning;
}

void taskJoin(Task_t task)
{
    TaskState_t* state;
    bool         joined = taskTryBeginJoin(task, &state);
    assert(joined);
    if (!joined)
    {
        return;
    }

    xEventGroupSetBits(state->eg, SIGNAL_BIT_MUST_QUIT);
    rundownProtRelease(&state->rundown);
    rundownProtWait(&state->rundown);
    taskDestroy(state);
}

void taskDetach(Task_t task)
{
    TaskState_t* state;
    bool         selfDetach;
    bool         detached = taskTryBeginDetach(task, &state, &selfDetach);
    assert(detached);
    if (!detached)
    {
        return;
    }

    if (selfDetach)
    {
        taskSetContinue(state);
        xEventGroupWaitBits(state->eg, SIGNAL_BIT_CREATOR_READY, pdFALSE, pdTRUE, portMAX_DELAY);
        rundownProtRelease(&state->rundown);
        rundownProtWait(&state->rundown);
        taskDestroy(state);
        return;
    }

    rundownProtRelease(&state->rundown);
    rundownProtWait(&state->rundown);
    taskDestroy(state);
}

// -----------------------------------------------------------------------------

static void taskRoutine(void* pvParameters)
{
    TaskState_t*       task = static_cast<TaskState_t*>(pvParameters);
    TaskLifecycle_t    expected = TaskLifecycle_t::Running;
    EventGroupHandle_t eg;

    atomic_store_explicit(&task->worker, xTaskGetCurrentTaskHandle(), memory_order_release);

    if (!task->pvParameters)
    {
        taskSetContinue(task);
    }

    task->fn(task, task->pvParameters);

    taskSetContinue(task);
    eg = task->eg;
    atomic_compare_exchange_strong_explicit(&task->lifecycle, &expected, TaskLifecycle_t::Exited, memory_order_acq_rel,
                                            memory_order_acquire);
    xEventGroupSetBits(eg, SIGNAL_BIT_QUIT);
    rundownProtRelease(&task->rundown);

    vTaskDelete(nullptr);
}

static void taskSetContinue(TaskState_t* task)
{
    xEventGroupSetBits(task->eg, SIGNAL_BIT_CONTINUE);
}

static void taskDestroy(TaskState_t* task)
{
    vEventGroupDelete(task->eg);
    task->eg = nullptr;
    rundownProtDestroy(&task->rundown);
    free(task);
}

static void taskRegister(TaskState_t* task)
{
    AutoMutex lock(sTaskRegistryMtx);

    task->next = sTaskRegistryHead;
    sTaskRegistryHead = task;
}

static void taskUnregister(TaskState_t* task)
{
    AutoMutex lock(sTaskRegistryMtx);

    taskUnregisterLocked(task);
}

static void taskUnregisterLocked(TaskState_t* task)
{
    TaskState_t** current = &sTaskRegistryHead;

    while ((*current) && (*current != task))
    {
        current = &(*current)->next;
    }

    assert(*current == task);
    if (*current == task)
    {
        *current = task->next;
        task->next = nullptr;
    }
}

static TaskState_t* taskFindRegistered(Task_t task)
{
    TaskState_t* current = sTaskRegistryHead;

    while (current)
    {
        if (current == task)
        {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

static bool taskTryAcquire(Task_t task, TaskState_t** state)
{
    AutoMutex    lock(sTaskRegistryMtx);
    TaskState_t* registered = taskFindRegistered(task);

    if (!registered || !rundownProtAcquire(&registered->rundown))
    {
        return false;
    }

    *state = registered;
    return true;
}

static bool taskTryBeginJoin(Task_t task, TaskState_t** state)
{
    AutoMutex       lock(sTaskRegistryMtx);
    TaskState_t*    registered = taskFindRegistered(task);
    TaskLifecycle_t lifecycle;
    TaskLifecycle_t expected;

    if (!registered)
    {
        return false;
    }

    lifecycle = atomic_load_explicit(&registered->lifecycle, memory_order_acquire);
    if ((lifecycle == TaskLifecycle_t::Running) &&
        (atomic_load_explicit(&registered->worker, memory_order_acquire) == xTaskGetCurrentTaskHandle()))
    {
        return false;
    }

    expected = TaskLifecycle_t::Running;
    if (!atomic_compare_exchange_strong_explicit(&registered->lifecycle, &expected, TaskLifecycle_t::Joining, memory_order_acq_rel,
                                                 memory_order_acquire))
    {
        expected = TaskLifecycle_t::Exited;
        if (!atomic_compare_exchange_strong_explicit(&registered->lifecycle, &expected, TaskLifecycle_t::Joining, memory_order_acq_rel,
                                                     memory_order_acquire))
        {
            return false;
        }
    }

    if (!rundownProtAcquire(&registered->rundown))
    {
        assert(false);
        return false;
    }

    taskUnregisterLocked(registered);
    *state = registered;
    return true;
}

static bool taskTryBeginDetach(Task_t task, TaskState_t** state, bool* selfDetach)
{
    AutoMutex       lock(sTaskRegistryMtx);
    TaskState_t*    registered = taskFindRegistered(task);
    TaskLifecycle_t lifecycle;
    TaskLifecycle_t expected;

    if (!registered)
    {
        return false;
    }

    lifecycle = atomic_load_explicit(&registered->lifecycle, memory_order_acquire);
    expected = TaskLifecycle_t::Running;
    *selfDetach = (lifecycle == TaskLifecycle_t::Running) &&
                  (atomic_load_explicit(&registered->worker, memory_order_acquire) == xTaskGetCurrentTaskHandle());
    if (*selfDetach)
    {
        if (!atomic_compare_exchange_strong_explicit(&registered->lifecycle, &expected, TaskLifecycle_t::Detached, memory_order_acq_rel,
                                                     memory_order_acquire))
        {
            return false;
        }
    }
    else
    {
        expected = TaskLifecycle_t::Exited;
        if (!atomic_compare_exchange_strong_explicit(&registered->lifecycle, &expected, TaskLifecycle_t::Detached, memory_order_acq_rel,
                                                     memory_order_acquire))
        {
            return false;
        }
        if (!rundownProtAcquire(&registered->rundown))
        {
            assert(false);
            return false;
        }
    }

    taskUnregisterLocked(registered);
    *state = registered;
    return true;
}
