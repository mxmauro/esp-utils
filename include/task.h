#pragma once

#include "run_once.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

// -----------------------------------------------------------------------------

// Task manages a FreeRTOS task with synchronization mechanisms.
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

void taskInit(Task_t *task);

// Creates a new task and waits until the continue event is signalled.
esp_err_t taskCreate(Task_t *task, TaskRoutine_t fn, const char *pcName, uint32_t usStackDepth,
                     void * pvParameters, UBaseType_t uxPriority, BaseType_t xCoreID);

// Call this function after the task copies the given parameters
void taskSignalContinue(Task_t *task);

// Checks if a task should quit.
bool taskShouldQuit(Task_t *task);

bool taskIsRunning(Task_t *task);

// Signals the task to end and waits until it quits
void taskJoin(Task_t *task);

// Use this function ONLY if no other task calls 'taskJoin' in order to free resources.
void taskDetach(Task_t *task);

#ifdef __cplusplus
}
#endif // __cplusplus
