#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "tasks/cli/include/task_cli.h"
#include "tasks/main/include/task_main.h"
#include "config.h"
#include "wifi.h"
#include "events.h"
#include "buttons.h"

// Log Tag
static const char* TAG = "Init";

/**
 * Main application entry point.
 */
void app_main()
{
    // Initialise
    ESP_LOGI(TAG, "TXLED System Startup");

    // Load configuration from flash
    config_load();

    // Initialise inputs
    buttons_init();

    // Start tasks
    xTaskCreate(&task_cli, "cli_task", 30000, NULL, 5, NULL);
    xTaskCreate(&task_main, "main_task", 30000, NULL, 5, NULL);
}