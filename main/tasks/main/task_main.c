#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "config.h"
#include "include/task_main.h"
#include "wifi.h"
#include "events.h"

// Log Tag
static const char* TAG = "Main";

/**
 * Main task.
 * Just serves as a heartbeat for now - flashing the status LED periodically.
 */
void task_main(void* params)
{
    // Create the event group for Wi-Fi state change events
    sys_event_group = xEventGroupCreate();

    // Start the Wi-Fi if possible
    wifi_init();

    // Spin and wait here
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}