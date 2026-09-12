#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>

// -----------------------------------------------------------------------------

// Opaque handle for a managed FreeRTOS task.
typedef void* Task_t;

typedef void (*TaskRoutine_t)(Task_t task, void* arg);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Creates a managed FreeRTOS task. Returns nullptr if an argument is invalid or
// if allocating its state, creating its event group, or creating the task fails.
// When pvParameters is non-null, this blocks until the callback signals that it
// has copied or no longer needs the caller-owned parameter storage.
Task_t taskCreate(TaskRoutine_t fn, const char* pcName, uint32_t usStackDepth, void* pvParameters, UBaseType_t uxPriority,
                  BaseType_t xCoreID);

// Signals that the managed callback copied its startup parameters and can
// continue. This function is only valid when called by that callback and is
// idempotent.
void taskSignalContinue(Task_t task);

// Reports whether the managed task has been asked to stop.
bool taskShouldQuit(Task_t task);

// Reports whether the managed task remains available for normal work. It
// returns false once joining or detachment has started.
bool taskIsRunning(Task_t task);

// Requests the managed task to stop, waits for it to exit, and consumes task.
// Set the caller's handle to nullptr immediately afterward and never use it
// again. The caller must serialize ownership transfer with taskDetach().
void taskJoin(Task_t task);

// Releases task after either of two supported ownership transfers:
// - the managed callback may detach itself, then may continue arbitrary
//   non-task work without using task and must eventually call vTaskDelete(nullptr)
//   instead of returning;
// - an external owner may detach only after the managed callback has returned.
//
// taskDetach() consumes task. Set the caller's handle to nullptr immediately
// afterward and never use it again. The caller must serialize ownership
// transfer with taskJoin().
void taskDetach(Task_t task);

#ifdef __cplusplus
}
#endif // __cplusplus
