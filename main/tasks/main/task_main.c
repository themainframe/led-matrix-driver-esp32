#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
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

// Buffer used for time text generation
static char strftime_buf[64];

/**
 * Main task.
 * Updates the clock display once a second.
 */
void task_main(void* params)
{
    ESP_LOGI(TAG, "Main task started");

    // Set up the heartbeat LED
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT_OD);

    // Start the Wi-Fi if possible
    // wifi_init();
    display_init(config_get_int(CONFIG_GCR));
    display_t display;

    // Update the display once a second
    for (uint i = 0; ; i ++)
    {
        // Get the time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        // Clear the display
        display_fill(&display, 0, 0);
        strftime(strftime_buf, sizeof(strftime_buf), "%H", &timeinfo);
        display_text(&display, 0, i % 2 == 0 ? 0xffffffff : 0x77777777, strftime_buf);
        strftime(strftime_buf, sizeof(strftime_buf), "%M", &timeinfo);
        display_text(&display, 11, i % 2 == 0 ? 0xffffffff : 0x77777777, strftime_buf);
        display_update(&display);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(PIN_LED, i % 2 == 0);
    }
}
