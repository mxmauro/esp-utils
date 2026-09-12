#include "reset_button.h"
#include "time.h"
#include <assert.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <string.h>

static const char* TAG = "RST-BTN";

#define LONG_PRESS_MS 3000 // hold 3s to reset
#define DEBOUNCE_MS   30

// -----------------------------------------------------------------------------

static gpio_num_t resetButtonGpioPin = GPIO_NUM_NC;

static ResetButtonPressedHandler_t handler = nullptr;
static void*                       handlerCtx = nullptr;

static TimerHandle_t    longPressTimer = nullptr;
static TaskHandle_t     actionTaskHandle = nullptr;
static volatile int     lastLevel = 1;
static volatile int64_t lastEdgeMs = 0;
static portMUX_TYPE     resetButtonLock = portMUX_INITIALIZER_UNLOCKED;
static bool             buttonPressed = false;
static int64_t          buttonPressStartedMs = 0;
static bool             initialized = false;

// -----------------------------------------------------------------------------

static void actionTask(void* arg);
static void onLongPressTimer(TimerHandle_t xTimer);
static void onButtonISR(void* arg);

// -----------------------------------------------------------------------------

void setupResetButton(gpio_num_t gpioPin, ResetButtonPressedHandler_t _handler, void* ctx)
{
    gpio_config_t gpioConfig;
    esp_err_t     err;

    assert(_handler);

    portENTER_CRITICAL(&resetButtonLock);
    if (initialized)
    {
        portEXIT_CRITICAL(&resetButtonLock);
        assert(false);
        return;
    }
    initialized = true;
    portEXIT_CRITICAL(&resetButtonLock);

    resetButtonGpioPin = gpioPin;
    handler = _handler;
    handlerCtx = ctx;

    // Create the action task
    ESP_ERROR_CHECK(xTaskCreate(actionTask, "rstBtnTask", 4096, nullptr, 5, &actionTaskHandle) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);

    // Create one-shot long-press timer
    longPressTimer = xTimerCreate("rstBtnTimer", pdMS_TO_TICKS(LONG_PRESS_MS), pdFALSE, nullptr, onLongPressTimer);
    ESP_ERROR_CHECK(longPressTimer ? ESP_OK : ESP_ERR_NO_MEM);

    // Configure button GPIO
    memset(&gpioConfig, 0, sizeof(gpioConfig));
    gpioConfig.pin_bit_mask = 1ULL << static_cast<int>(resetButtonGpioPin);
    gpioConfig.mode = GPIO_MODE_INPUT;
    gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
    gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpioConfig.intr_type = GPIO_INTR_ANYEDGE;
    ESP_ERROR_CHECK(gpio_config(&gpioConfig));

    // Install ISR and attach handler
    err = gpio_install_isr_service(0);
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE))
    {
        ESP_ERROR_CHECK(err);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(resetButtonGpioPin, onButtonISR, nullptr));

    ESP_LOGI(TAG, "Reset button controller sucessfully initialized.");
}

// -----------------------------------------------------------------------------

static void actionTask(void* arg)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Reset button pressed!");

        handler(handlerCtx);
    }
}

static void onLongPressTimer(TimerHandle_t xTimer)
{
    // Timer runs in timer task context — don’t do heavy work here.
    bool    pressed;
    int64_t startedMs;

    portENTER_CRITICAL(&resetButtonLock);
    pressed = buttonPressed;
    startedMs = buttonPressStartedMs;
    portEXIT_CRITICAL(&resetButtonLock);

    if (pressed && ((now_ms() - startedMs) >= LONG_PRESS_MS))
    {
        xTaskNotifyGive(actionTaskHandle);
    }
}

static void IRAM_ATTR onButtonISR(void* arg)
{
    const int     level = gpio_get_level(resetButtonGpioPin);
    const int64_t now = now_ms();

    // Debounce: ignore edges that occur too soon after the last one
    if (now - lastEdgeMs < DEBOUNCE_MS)
    {
        return;
    }
    lastEdgeMs = now;

    // Only react to real level changes
    if (level == lastLevel)
    {
        return;
    }
    lastLevel = level;

    BaseType_t hpw = pdFALSE;
    if (level == 0)
    {
        portENTER_CRITICAL_ISR(&resetButtonLock);
        buttonPressed = true;
        buttonPressStartedMs = now;
        portEXIT_CRITICAL_ISR(&resetButtonLock);

        // Reset starts the one-shot timer with a single queue command.
        assert(xTimerResetFromISR(longPressTimer, &hpw) == pdPASS);
    }
    else
    {
        portENTER_CRITICAL_ISR(&resetButtonLock);
        buttonPressed = false;
        portEXIT_CRITICAL_ISR(&resetButtonLock);

        // A pending callback checks buttonPressed before notifying the task.
        assert(xTimerStopFromISR(longPressTimer, &hpw) == pdPASS);
    }
    if (hpw)
    {
        portYIELD_FROM_ISR();
    }
}
