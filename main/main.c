#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tasks/cli/include/task_cli.h"
#include "tasks/main/include/task_main.h"
#include "tasks/display/include/task_display.h"
#include "config.h"
#include "buttons.h"
#include "frame_buffer.h"

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

    // Initialise framebuffer
    fb_init();

    // Start tasks
    xTaskCreatePinnedToCore(&task_cli, "cli_task", 30000, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&task_main, "main_task", 30000, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&task_display, "display_task", 30000, NULL, 5, NULL, 1);
}