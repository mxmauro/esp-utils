#include "task.h"
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

// static const char* TAG = "Task";

#define SIGNAL_BIT_CONTINUE  1
#define SIGNAL_BIT_MUST_QUIT 2
#define SIGNAL_BIT_QUIT      4

// -----------------------------------------------------------------------------

typedef struct InternalTaskData_s {
    Task_t *task;
    TaskRoutine_t fn;
    void *pvParameters;
} InternalTaskData_t;

// -----------------------------------------------------------------------------

static void taskRoutine(void *pvParameters);

// -----------------------------------------------------------------------------

void taskInit(Task_t *task)
{
    runOnceInit(&task->once);
    task->eg = nullptr;
    memset(&task->eventGroupBuffer, 0, sizeof(task->eventGroupBuffer));
}

bool taskCreate(Task_t *task, TaskRoutine_t fn, const char *pcName, uint32_t usStackDepth,
                void *pvParameters, UBaseType_t uxPriority, BaseType_t xCoreID)
{
    InternalTaskData_t data = {
        .task = task,
        .fn = fn,
        .pvParameters = pvParameters
    };
    TaskHandle_t h;

    runOnceInit(&task->once);
    task->eg = xEventGroupCreateStatic(&(task->eventGroupBuffer));

    if (xTaskCreatePinnedToCore(&taskRoutine, pcName, usStackDepth, &data, uxPriority, &h, xCoreID) != pdPASS) {
        vEventGroupDelete(task->eg);
        task->eg = nullptr;
        return false;
    }

    // Wait for the continue flag
    xEventGroupWaitBits(task->eg, SIGNAL_BIT_CONTINUE, pdFALSE, pdTRUE, portMAX_DELAY);

    // Done
    return true;
}

void taskSignalContinue(Task_t *task)
{
    // Signal continue
    xEventGroupSetBits(task->eg, SIGNAL_BIT_CONTINUE);
}

bool taskShouldQuit(Task_t *task)
{
    // Check for quit
    if (!task->eg) {
        return true;
    }
    return ((xEventGroupWaitBits(task->eg, SIGNAL_BIT_MUST_QUIT, pdFALSE, pdFALSE, 0) & SIGNAL_BIT_MUST_QUIT) != 0);
}

bool taskIsRunning(Task_t *task)
{
    if (!task->eg) {
        return false;
    }
    return ((xEventGroupWaitBits(task->eg, SIGNAL_BIT_QUIT, pdFALSE, pdFALSE, 0) & SIGNAL_BIT_QUIT) == 0);
}

void taskJoin(Task_t *task)
{
    runOnce(&task->once, [](const void *arg) -> void {
        Task_t *task = (Task_t *)arg;

        if (task->eg) {
            // Signal quit
            xEventGroupSetBits(task->eg, SIGNAL_BIT_MUST_QUIT);

            // Wait for quit
            xEventGroupWaitBits(task->eg, SIGNAL_BIT_QUIT, pdFALSE, pdFALSE, portMAX_DELAY);

            // Delete event group
            vEventGroupDelete(task->eg);
            task->eg = nullptr;
            memset(&task->eventGroupBuffer, 0, sizeof(task->eventGroupBuffer));
        }
    }, task);
}

void taskDetach(Task_t *task)
{
    if (task->eg) {
        // Delete event group
        vEventGroupDelete(task->eg);
        task->eg = nullptr;
        memset(&task->eventGroupBuffer, 0, sizeof(task->eventGroupBuffer));
    }
}

static void taskRoutine(void *pvParameters)
{
    InternalTaskData_t data;

    memcpy(&data, (InternalTaskData_t *)pvParameters, sizeof(InternalTaskData_t));

    // ESP_LOGD(TAG, "On task routine.");

    // If no parameters were given, no need to ask the user to signal continue
    if (!data.pvParameters) {
        taskSignalContinue(data.task);
    }

    // ESP_LOGD(TAG, "Task before callback.");

    // Call the original callback
    data.fn(data.task, data.pvParameters);

    // ESP_LOGD(TAG, "Task after callback.");

    // Signal quit
    if (data.task->eg) {
        xEventGroupSetBits(data.task->eg, SIGNAL_BIT_QUIT);
    }

    // Delete task
    vTaskDelete(nullptr);
}
