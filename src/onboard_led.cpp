#include "onboard_led.h"

#if CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812
#include <assert.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>
#endif // CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812

#if CONFIG_BOARD_LED_TYPE_WS2812
#include <driver/rmt_encoder.h>
#include <driver/rmt_tx.h>
#include <soc/soc_caps.h>

#if !SOC_RMT_SUPPORTED
    #error "WS2812 onboard LED requires RMT support"
#endif // !SOC_RMT_SUPPORTED
#endif // CONFIG_BOARD_LED_TYPE_WS2812

// -----------------------------------------------------------------------------

#if CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812
static StaticSemaphore_t ledMutexBuffer;
static SemaphoreHandle_t ledMutex;

#if CONFIG_BOARD_LED_TYPE_GPIO
#if CONFIG_BOARD_LED_ACTIVE_LOW
static constexpr bool ledActiveLow = true;
#else // CONFIG_BOARD_LED_ACTIVE_LOW
static constexpr bool ledActiveLow = false;
#endif // CONFIG_BOARD_LED_ACTIVE_LOW
#endif // CONFIG_BOARD_LED_TYPE_GPIO

#if CONFIG_BOARD_LED_TYPE_WS2812
static rmt_channel_handle_t ledChannel;
static rmt_encoder_handle_t ledEncoder;
#endif // CONFIG_BOARD_LED_TYPE_WS2812
#endif // CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812

// -----------------------------------------------------------------------------

#if CONFIG_BOARD_LED_TYPE_WS2812
static void setColor(uint8_t red, uint8_t green, uint8_t blue);
#endif // CONFIG_BOARD_LED_TYPE_WS2812


// -----------------------------------------------------------------------------

void onboardLedInit()
{
#if CONFIG_BOARD_LED_TYPE_WS2812
    rmt_tx_channel_config_t txConfig;
    rmt_copy_encoder_config_t encoderConfig;
#endif // CONFIG_BOARD_LED_TYPE_WS2812

#if CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812
    if (ledMutex)
    {
        return; // Already initialized
    }
    assert(GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_BOARD_LED_GPIO));

    ledMutex = xSemaphoreCreateMutexStatic(&ledMutexBuffer);
    assert(ledMutex);

#if CONFIG_BOARD_LED_TYPE_GPIO
    ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(CONFIG_BOARD_LED_GPIO), ledActiveLow ? 1 : 0));
    ESP_ERROR_CHECK(gpio_set_direction(static_cast<gpio_num_t>(CONFIG_BOARD_LED_GPIO), GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(static_cast<gpio_num_t>(CONFIG_BOARD_LED_GPIO), GPIO_FLOATING));
#endif // CONFIG_BOARD_LED_TYPE_GPIO

#if CONFIG_BOARD_LED_TYPE_WS2812
    memset(&txConfig, 0, sizeof(txConfig));
    txConfig.gpio_num = static_cast<gpio_num_t>(CONFIG_BOARD_LED_GPIO);
    txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
    txConfig.resolution_hz = 10000000;
    txConfig.mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL;
    txConfig.trans_queue_depth = 1;
    memset(&encoderConfig, 0, sizeof(encoderConfig));

    ESP_ERROR_CHECK(rmt_new_tx_channel(&txConfig, &ledChannel));
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoderConfig, &ledEncoder));
    ESP_ERROR_CHECK(rmt_enable(ledChannel));

    setColor(0, 0, 0);
#endif // CONFIG_BOARD_LED_TYPE_WS2812
#endif // CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812
}

void onboardLedSet(bool on)
{
#if CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812
    if (ledMutex)
    {
        xSemaphoreTake(ledMutex, portMAX_DELAY);

#if CONFIG_BOARD_LED_TYPE_GPIO
        (void)gpio_set_level(static_cast<gpio_num_t>(CONFIG_BOARD_LED_GPIO), on != ledActiveLow);
#endif // CONFIG_BOARD_LED_TYPE_GPIO
#if CONFIG_BOARD_LED_TYPE_WS2812
        setColor(0, 0, (on) ? 255 : 0);
#endif // CONFIG_BOARD_LED_TYPE_WS2812

        xSemaphoreGive(ledMutex);
    }
#else // CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812
    (void)on;
#endif // CONFIG_BOARD_LED_TYPE_GPIO || CONFIG_BOARD_LED_TYPE_WS2812
}

#if CONFIG_BOARD_LED_TYPE_WS2812
void onboardLedSetRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    if (ledMutex)
    {
        xSemaphoreTake(ledMutex, portMAX_DELAY);
        setColor(red, green, blue);
        xSemaphoreGive(ledMutex);
    }
}
#endif

// -----------------------------------------------------------------------------

#if CONFIG_BOARD_LED_TYPE_WS2812
static void setColor(uint8_t red, uint8_t green, uint8_t blue)
{
#if CONFIG_BOARD_LED_CHANNEL_MAP_RGB
    const uint8_t channels[] = {red, green, blue};
#elif CONFIG_BOARD_LED_CHANNEL_MAP_RBG
    const uint8_t channels[] = {red, blue, green};
#elif CONFIG_BOARD_LED_CHANNEL_MAP_GRB
    const uint8_t channels[] = {green, red, blue};
#elif CONFIG_BOARD_LED_CHANNEL_MAP_GBR
    const uint8_t channels[] = {green, blue, red};
#elif CONFIG_BOARD_LED_CHANNEL_MAP_BRG
    const uint8_t channels[] = {blue, red, green};
#else
    const uint8_t channels[] = {blue, green, red};
#endif
    rmt_symbol_word_t     symbols[25] = {};
    rmt_transmit_config_t config = {};

    for (unsigned i = 0; i < 24; ++i)
    {
        bool bit = (channels[i / 8] & (0x80U >> (i % 8))) != 0;
        symbols[i].level0 = 1;
        symbols[i].duration0 = bit ? 9 : 3;
        symbols[i].level1 = 0;
        symbols[i].duration1 = bit ? 3 : 9;
    }
    // The 50 us low symbol latches the frame at a 10 MHz resolution.
    symbols[24].duration0 = 250;
    symbols[24].duration1 = 250;
    ESP_ERROR_CHECK(rmt_transmit(ledChannel, ledEncoder, symbols, sizeof(symbols), &config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(ledChannel, -1));
}
#endif // CONFIG_BOARD_LED_TYPE_WS2812
