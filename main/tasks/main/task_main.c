#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "include/task_main.h"
#include "driver/gpio.h"
#include "config.h"
#include "wifi.h"
#include "pins.h"
#include "is32.h"

// Log Tag
static const char* TAG = "Main";

/**
 * Main task.
 * Just serves as a heartbeat for now - flashing the status LED periodically.
 */
void task_main(void* params)
{
    ESP_LOGI(TAG, "Main task started");

    // Start the Wi-Fi if possible
    // wifi_init();
    is32_init();

    // Set up the heartbeat LED
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT_OD);

    // Spin and wait here
    for (uint i = 0; ; i ++)
    {
        vTaskDelay(750 / portTICK_PERIOD_MS);
        gpio_set_level(PIN_LED, i % 2 == 0);
        is32_write_reg(IS32_ADDRESS_A, IS32_REG_CONFIG, IS32_SSD_RUN);
        is32_write_reg(IS32_ADDRESS_A, IS32_REG_GLOBAL_CURRENT_CONTROL, 0xFF);
    }
}