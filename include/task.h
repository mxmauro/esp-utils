#pragma once

#include "run_once.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

// -----------------------------------------------------------------------------

// Stores the synchronization state used to manage a FreeRTOS task.
typedef struct Task_s {
    RunOnce_t once;
    EventGroupHandle_t eg;
    StaticEventGroup_t eventGroupBuffer;
} Task_t;

typedef void (*TaskRoutine_t)(Task_t *task, void *arg);

#define TASK_INIT_STATIC { .once = RUN_ONCE_INIT_STATIC, .eg = nullptr, .eventGroupBuffer = {} }

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initializes a task management state object.
void taskInit(Task_t *task);

// Creates a managed FreeRTOS task and waits for its startup handshake.
esp_err_t taskCreate(Task_t *task, TaskRoutine_t fn, const char *pcName, uint32_t usStackDepth,
                     void * pvParameters, UBaseType_t uxPriority, BaseType_t xCoreID);

// Signals that the task copied its startup parameters and can continue.
void taskSignalContinue(Task_t *task);

// Reports whether the managed task has been asked to stop.
bool taskShouldQuit(Task_t *task);

// Reports whether the managed task is currently running.
bool taskIsRunning(Task_t *task);

// Requests the managed task to stop and waits for it to exit.
void taskJoin(Task_t *task);

// Detaches the task from this Task_t instance and releases the event-group state
// stored in the struct. The caller becomes responsible for any remaining task or
// resource lifetime management and must ensure no other code keeps using this Task_t.
void taskDetach(Task_t *task);

#ifdef __cplusplus
}
#endif // __cplusplus
