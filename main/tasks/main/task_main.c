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
#include "buttons.h"
#include "transition.h"
#include "frame_buffer.h"

// Log Tag
static const char* TAG = "Main";

/**
 * Main task.
 */
void task_main(void* params)
{
    ESP_LOGI(TAG, "Main task started");

    // Set up the heartbeat LED
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT_OD);

    // Start the Wi-Fi if possible
    // wifi_init();

    // Start the display engine
    display_init(config_get_int(CONFIG_GCR));

    display_t display_blank;
    display_t display_left;
    display_t display_right;
    display_t display_cb;

    display_fill(&display_blank, 0x00, true);
    display_fill(&display_left, 0x00, true);
    display_fill(&display_right, 0x00, true);
    display_rect(&display_left, 0, 0, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT, 0xff, true);
    display_rect(&display_right, DISPLAY_WIDTH / 2, 0, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT, 0xff, true);

    // fb_push(&display_left);

    // while(true) {

    //     trans_handle_t* fade_l_to_r = trans_fade(&display_left, &display_right, 64);
    //     while(!(trans_progress(fade_l_to_r)->is_finished)) {
    //         fb_push(fade_l_to_r->current);
    //         vTaskDelay(30 / portTICK_PERIOD_MS);
    //     }
    //     trans_free(fade_l_to_r);

    //     trans_handle_t* fade_r_to_l = trans_fade(&display_right, &display_left, 64);
    //     while(!(trans_progress(fade_r_to_l)->is_finished)) {
    //         fb_push(fade_r_to_l->current);
    //         vTaskDelay(30 / portTICK_PERIOD_MS);
    //     }
    //     trans_free(fade_r_to_l);
    // }


    while(true) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Checkerboard...");
        display_checkerboard(&display_cb, false, 0xff);
        fb_push(&display_cb);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Inverse checkerboard...");
        display_checkerboard(&display_cb, true, 0xff);
        fb_push(&display_cb);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Scroll text
        const char* text = "hello ~ transition test =)";
        trans_handle_t* scroll = trans_scroll_text(text, false, SCROLL_START_CLEAR, SCROLL_END_FULL);
        while(!(trans_progress(scroll)->is_finished)) {
            fb_push(scroll->current);
            vTaskDelay(75 / portTICK_PERIOD_MS);
        }

        // Now wipe to checkerboard
        trans_handle_t* wipe = trans_wipe(scroll->current, &display_cb, WIPE_DOWN);
        while(!(trans_progress(wipe)->is_finished)) {
            fb_push(wipe->current);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // Fade to black
        trans_handle_t* fade = trans_fade(wipe->current, &display_blank, 64);
        while(!(trans_progress(fade)->is_finished)) {
            fb_push(fade->current);
            vTaskDelay(30 / portTICK_PERIOD_MS);
        }

        fb_push(fade->current);
        trans_free(scroll);
        trans_free(wipe);
        trans_free(fade);

    }    
}
