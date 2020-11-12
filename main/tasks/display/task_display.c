#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "include/task_display.h"
#include "driver/gpio.h"
#include "pins.h"
#include "config.h"
#include "display.h"
#include "buttons.h"
#include "frame_buffer.h"

// Log Tag
static const char* TAG = "DispTask";

/**
 * Display task.
 * Syncs the display to the desired state.
 */
void task_display()
{
    ESP_LOGI(TAG, "Display driver task started");

    // Set up the heartbeat LED
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT_OD);

    // Start the display engine
    display_init(config_get_int(CONFIG_GCR));

    while(true) {
        fb_write();
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }

}
