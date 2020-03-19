#include <stdint.h>
#include <stdlib.h>
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "freertos/event_groups.h"
#include "include/task_cli.h"
#include "driver/gpio.h"
#include "config.h"

static const char* TAG = "CLI";

/**
 * Update a configuration item.
 */
static int cmd_set(int argc, char** argv)
{
    if (argc < 3) {
        ESP_LOGW(TAG, "Insufficient arguments supplied - expected: key value");
        return -1;
    }

    return config_set(argv[1], argv[2]);
}

/**
 * Get a configuration item.
 */
static int cmd_get(int argc, char** argv)
{
    const char* value;

    // If no argument was specified, dump everything
    if (argc < 2) {   
        
        // Dump all configuration values
        for (int cfg_idx = 0; cfg_idx < config_count; cfg_idx ++) {
            if ((value = config_get(config[cfg_idx].key)) == NULL) {
                continue;
            }
            ESP_LOGI(TAG, "%s => %s", config[cfg_idx].key, value);
        }

        return 0;
    }

    // Look for a specific value
    value = config_get(argv[1]);

    if (value == NULL) {
        return -1;
    }

    ESP_LOGI(TAG, "%s: %s", argv[1], value);
    return 0;
}

/**
 * Save configuration values to non-volatile storage.
 */
static int cmd_save(int argc, char** argv)
{
    return config_save();
}

/**
 * Change a GPIO pad state.
 */
static int cmd_gpio(int argc, char** argv)
{
    if (argc < 3) {
        ESP_LOGW(TAG, "Insufficient arguments supplied: expected GPIO, state.");
        return -1;
    }

    gpio_pad_select_gpio(atoi(argv[1]));
    gpio_set_direction(atoi(argv[1]), GPIO_MODE_OUTPUT);
    gpio_set_level(atoi(argv[1]), atoi(argv[2]));

    return 0;
}

/**
 * Reset the system.
 */
static int cmd_reset(int argc, char** argv)
{
    esp_restart();
    return 0;
}

/**
 * Initialise the console.
 */
static void init_console()
{
    /* Disable buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);

    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .use_ref_tick = true
    };
    ESP_ERROR_CHECK( uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config) );

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
            .hint_color = atoi(LOG_COLOR_CYAN)
    };
    
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);
}

/**
 * CLI Task.
 * 
 * This task handles the command line interface.
 */
void task_cli()
{
    // Initialise the console
    init_console();

    // Register commands
    esp_console_register_help_command();

    const esp_console_cmd_t cmd_set_spec = {
        .command = "set",
        .help = "Set a configuration value.",
        .hint = NULL,
        .func = &cmd_set,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_set_spec));

    const esp_console_cmd_t cmd_get_spec = {
        .command = "get",
        .help = "Get a configuration value.",
        .hint = NULL,
        .func = &cmd_get,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_get_spec));

    const esp_console_cmd_t cmd_save_spec = {
        .command = "save",
        .help = "Save configuration values to non-volatile storage.",
        .hint = NULL,
        .func = &cmd_save,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_save_spec));

    const esp_console_cmd_t cmd_gpio_spec = {
        .command = "gpio",
        .help = "Change a GPIO state",
        .hint = NULL,
        .func = &cmd_gpio,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_gpio_spec));

    const esp_console_cmd_t cmd_reset_spec = {
        .command = "reset",
        .help = "Reset the system",
        .hint = NULL,
        .func = &cmd_reset,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_reset_spec));

    // Figure out if the terminal supports escape sequences
    printf("\nTXLED Command Interface.\nType 'help' to get the list of commands.\n\n");
    int probe_status = linenoiseProbe();
    if (probe_status) {
        linenoiseSetDumbMode(1);
    }

    // Main loop
    while (true)
    {
        char* line = linenoise(LOG_COLOR_I "txled> " LOG_RESET_COLOR);

        if (line == NULL) {
            continue;
        }

        linenoiseHistoryAdd(line);

        // Try to run the command 
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command: %s\n", line);
        } else if (err == ESP_ERR_INVALID_ARG) {
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }

        // linenoise allocates line buffer on the heap, so need to free it 
        linenoiseFree(line);
    }
}
