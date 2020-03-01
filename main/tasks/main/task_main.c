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
#include "display.h"

// Log Tag
static const char* TAG = "Main";

/**
 * Main task.
 * Just serves as a heartbeat for now - flashing the status LED periodically.
 */
void task_main(void* params)
{
    ESP_LOGI(TAG, "Main task started");

    // Set up the heartbeat LED
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT_OD);

    // Start the Wi-Fi if possible
    // wifi_init();
    display_init();

    char hex[5] = "0000";
    display_t display;

    // Spin and wait here
    for (uint i = 0; ; i ++)
    {
        display_fill(&display, 0, 0);
        sprintf(hex, "%04X", i);
        display_text(&display, 0, 0x10, hex);
        display_update(&display);
        vTaskDelay(100 / portTICK_PERIOD_MS);

        gpio_set_level(PIN_LED, i % 2 == 0);
    }
}
