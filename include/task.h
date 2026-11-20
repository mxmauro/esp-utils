#pragma once

#ifdef __cplusplus
    #include <assert.h>
#endif // __cplusplus

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>
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

// -----------------------------------------------------------------------------

#ifdef __cplusplus

class Task
{
public:
    Task() noexcept = default;
    Task(const Task&) noexcept = delete;
    Task(Task&& other) noexcept : task(other.task)
    {
        other.task = nullptr;
    }

    // Requires explicit ownership completion through Join() or Detach().
    ~Task() noexcept
    {
        assert(task == nullptr);
    }

    Task& operator=(const Task&) noexcept = delete;
    Task& operator=(Task&& other) noexcept
    {
        if (this != &other)
        {
            assert(task == nullptr);
            task = other.task;
            other.task = nullptr;
        }
        return *this;
    }

    // Creates a managed task using the same callback contract as taskCreate().
    static Task Create(TaskRoutine_t fn, const char* pcName, uint32_t usStackDepth, void* pvParameters, UBaseType_t uxPriority,
                       BaseType_t xCoreID) noexcept
    {
        return Task(taskCreate(fn, pcName, usStackDepth, pvParameters, uxPriority, xCoreID));
    }

    // Reports whether this object owns a managed task handle.
    explicit operator bool() const noexcept
    {
        return task != nullptr;
    }

    // Signals that the managed callback no longer needs its startup parameters.
    void SignalContinue() noexcept
    {
        taskSignalContinue(task);
    }

    // Reports whether the managed task has been asked to stop.
    bool ShouldQuit() const noexcept
    {
        return taskShouldQuit(task);
    }

    // Reports whether the managed task remains available for normal work.
    bool IsRunning() const noexcept
    {
        return taskIsRunning(task);
    }

    // Requests shutdown, waits for completion, and consumes this owner's handle.
    void Join() noexcept
    {
        assert(task);
        Task_t ownedTask = task;

        task = nullptr;
        taskJoin(ownedTask);
    }

    // Transfers ownership to the managed or completed task and consumes this handle.
    void Detach() noexcept
    {
        assert(task);
        Task_t ownedTask = task;

        task = nullptr;
        taskDetach(ownedTask);
    }

    // Returns the underlying C handle without transferring ownership.
    Task_t NativeHandle() const noexcept
    {
        return task;
    }

private:
    explicit Task(Task_t _task) noexcept : task(_task)
    {
    }

private:
    Task_t task{nullptr};
};

#endif // __cplusplus
