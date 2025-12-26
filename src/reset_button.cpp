#include "reset_button.h"
#include "time.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>

static const char* TAG = "RST-BTN";

#define LONG_PRESS_MS 3000 // hold 3s to reset
#define DEBOUNCE_US   30

// -----------------------------------------------------------------------------

static gpio_num_t resetButtonGpioPin = GPIO_NUM_NC;
static ResetButtonPressedHandler_t callback = nullptr;
static TimerHandle_t longPressTimer = nullptr;
static TaskHandle_t actionTaskHandle = nullptr;
static volatile int lastLevel = 1;
static volatile int64_t lastEdgeUs = 0;

// -----------------------------------------------------------------------------

static void actionTask(void *arg);
static void onLongPressTimer(TimerHandle_t xTimer);
static void onButtonISR(void *arg);

// -----------------------------------------------------------------------------

void setupResetButton(gpio_num_t gpioPin, ResetButtonPressedHandler_t handler)
{
    resetButtonGpioPin = gpioPin;
    callback = handler;

    // Create the action task
    ESP_ERROR_CHECK((xTaskCreatePinnedToCore(actionTask, "rstBtnTask", 4096, nullptr, 5, &actionTaskHandle, 0) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM));

    // Create one-shot long-press timer
    longPressTimer = xTimerCreate("rstBtnTimer", pdMS_TO_TICKS(LONG_PRESS_MS), pdFALSE, nullptr, onLongPressTimer);

    // Configure button GPIO
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << (int)resetButtonGpioPin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    // Install ISR and attach handler
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(resetButtonGpioPin, onButtonISR, NULL));

    ESP_LOGI(TAG, "Reset button controller sucessfully initialized.");
}

// -----------------------------------------------------------------------------

static void actionTask(void *arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Reset button pressed!");

        callback();
    }
}

static void onLongPressTimer(TimerHandle_t xTimer)
{
    // Timer runs in timer task context — don’t do heavy work here.
    xTaskNotifyGive(actionTaskHandle);
}

static void IRAM_ATTR onButtonISR(void *arg)
{
    const int level = gpio_get_level(resetButtonGpioPin);
    const int64_t now = now_ms();

    // Debounce: ignore edges that occur too soon after the last one
    if (now - lastEdgeUs < DEBOUNCE_US) {
        return;
    }
    lastEdgeUs = now;

    // Only react to real level changes
    if (level == lastLevel) {
        return;
    }
    lastLevel = level;

    BaseType_t hpw = pdFALSE;
    if (level == 0) {
        // Button pressed: (re)start one-shot timer
        xTimerStopFromISR(longPressTimer, &hpw);
        xTimerChangePeriodFromISR(longPressTimer, pdMS_TO_TICKS(LONG_PRESS_MS), &hpw);
        xTimerStartFromISR(longPressTimer, &hpw);
    }
    else {
        // Button released: cancel the long-press
        xTimerStopFromISR(longPressTimer, &hpw);
    }
    if (hpw) {
        portYIELD_FROM_ISR();
    }
}
