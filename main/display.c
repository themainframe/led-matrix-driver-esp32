/*

The display is arranged like this, with chip resposibility represented by #, @ and X

    # # # # # # # # @ @ @ @ @ @ @ @ X X X X X X X X
    # # # # # # # # @ @ @ @ @ @ @ @ X X X X X X X X
    # # # # # # # # @ @ @ @ @ @ @ @ X X X X X X X X
    # # # # # # # # @ @ @ @ @ @ @ @ X X X X X X X X
    # # # # # # # # @ @ @ @ @ @ @ @ X X X X X X X X

*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "display.h"
#include "is32.h"
#include "pins.h"
#include "font_4x5.h"

static const char* TAG = "Dspl";

// Define the chip addresses that make up the display
is32_addr_t chip_addrs[IS32_CHIPS] = {
    IS32_ADDRESS_A,
    IS32_ADDRESS_B,
    IS32_ADDRESS_C
};

/**
 * Initialise the IS32 chips that make up the display.
 */
void display_init(int gcr)
{
    // Hardware shutdown control
    gpio_pad_select_gpio(PIN_SHUTDOWN);
    gpio_set_pull_mode(PIN_SHUTDOWN, GPIO_PULLUP_ONLY);
    gpio_set_direction(PIN_SHUTDOWN, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_SHUTDOWN, 1);

    // Initialise chip driver
    is32_init();

    // For each of the chips that control the entire display
    for (uint chip = 0; chip < IS32_CHIPS; chip ++) {

        // Set run mode
        is32_write_reg(chip_addrs[chip], IS32_REG_CONFIG, IS32_SSD_RUN | (chip == 0 ? IS32_SYNC_MASTER : IS32_SYNC_SLAVE));

        // Set the GCR
        is32_write_reg(chip_addrs[chip], IS32_REG_GLOBAL_CURRENT_CONTROL, gcr);

    }
}

/**
 * Write the display.
 */
void display_update(display_t* display)
{
    // PWM data for a single chip - 4 bytes per LED
    uint8_t chip_pwm[IS32_WIDTH * IS32_HEIGHT * 4] = {0};

    // On-Off data for a single chip - 4 bits per LED
    uint8_t chip_on_off[(IS32_WIDTH * IS32_HEIGHT) / 2] = {0};

    // For each of the chips that control the entire display
    for (uint chip = 0; chip < IS32_CHIPS; chip ++) {

        for (uint row = 0; row < DISPLAY_HEIGHT; row ++) {
            
            // Calculate the last column for this chip, which may be bounded by the display width
            uint last_col = (IS32_LAST_COL(chip) > DISPLAY_WIDTH) ? DISPLAY_WIDTH : IS32_LAST_COL(chip);

            for (uint col = IS32_FIRST_COL(chip); col < last_col; col ++) {

                uint chip_col = col - IS32_FIRST_COL(chip);

                // Just use the lowest PWM byte and spread it to the other three
                uint8_t pwm = (*display)[DISPLAY_WIDTH - 1 - col][row].pwm > 0xff ? 0xff : (*display)[DISPLAY_WIDTH - 1 - col][row].pwm & 0xff;

                // Build a list of PWM bytes to write to the chip
                chip_pwm[(chip_col * 2) + (row * 32)] = pwm;
                chip_pwm[(chip_col * 2) + (row * 32) + 1] = pwm;
                chip_pwm[(chip_col * 2) + (row * 32) + 16] = pwm;
                chip_pwm[(chip_col * 2) + (row * 32) + 17] = pwm;

                // Build the on-off bytes
                if ((*display)[DISPLAY_WIDTH - 1 - col][row].on) {
                    chip_on_off[(chip_col / 4) + (row * 4)] |= (0b11 << ((chip_col % 4) * 2));
                    chip_on_off[(chip_col / 4) + (row * 4) + 2] |= (0b11 << ((chip_col % 4) * 2));
                }

            }
        }

        // Write the chip's PWM register
        is32_write_seq(chip_addrs[chip], IS32_REG_PWM_START, chip_pwm, IS32_WIDTH * IS32_HEIGHT * 4);

        // Write the chip's LED I/O register
        is32_write_seq(chip_addrs[chip], IS32_REG_LED_ON_OFF_START, chip_on_off, (IS32_WIDTH * IS32_HEIGHT) / 2);

        // Clear chip states ready for the next chip
        memset(&chip_pwm, 0x00, sizeof(chip_pwm));
        memset(&chip_on_off, 0x00, sizeof(chip_on_off));
    }

    return;
}

/**
 * Render text onto a display at a given x-coordinate with a given PWM intensity.
 */
void display_text(display_t* display, int x_pos, uint32_t pwm, const char* text)
{
    // For each char in the requested text
    for (int i = 0; i < strlen(text); i ++) {

        for (int c_row = 0; c_row < 5; c_row ++) {
            for (int c_col = 0; c_col < 4; c_col ++) {

                // Calculate the X and Y positions for this pixel of the character
                int col = x_pos + c_col;

                // Bounds check for this pixel
                if (col < 0 || col >= DISPLAY_WIDTH) {
                    // Don't try to draw this pixel, it's out of bounds
                    continue;
                }

                // Set this pixel?
                if (font_4x5[(int)text[i]][c_row] & (1 << (3 - c_col))) {
                    (*display)[col][c_row].on = true;
                    (*display)[col][c_row].pwm = pwm;
                }
            }
        }
        
        // 1-dot space between letters
        x_pos += 5;
    }
}

/**
 * Fill a rectangular area of the display with a given state.
 */
void display_rect(display_t* display, int x_pos, int y_pos, int width, int height, uint32_t pwm, bool on)
{
    for (int x = x_pos; x < x_pos + width; x ++) {
        for (int y = y_pos; y < y_pos + height; y ++) {
            
            // Within bounds?
            if (x < 0 || y < 0 || x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
                continue;
            }

            (*display)[x][y].on = true;
            (*display)[x][y].pwm = pwm;
        }
    }
}

/**
 * Copy a rectangular section of one display state to another.
 * Copying sections with the same source and destination is supported.
 */
void display_copy(const display_t* source, display_t* dest, int src_x_pos, int src_y_pos, int dest_x_pos, int dest_y_pos, int width, int height)
{
    ESP_LOGI(TAG, "copy: %p to %p (src: %d,%d, dst: %d,%d, size:%dby%d)", source, dest, src_x_pos, src_y_pos, dest_x_pos, dest_y_pos, width, height);
    display_t* source_copy = malloc(sizeof(display_t));
    memcpy(source_copy, source, sizeof(display_t));

    for (int x = 0; x < width; x ++) {
        for (int y = 0; y < height; y ++) {
            
            int src_x = x + src_x_pos;
            int src_y = y + src_y_pos;

            // Source within bounds?
            if (src_x < 0 || src_y < 0 || src_x >= DISPLAY_WIDTH || src_y >= DISPLAY_HEIGHT) {
                continue;
            }

            int dest_x = x + dest_x_pos;
            int dest_y = y + dest_y_pos;

            // Destination within bounds?
            if (dest_x < 0 || dest_y < 0 || dest_x >=DISPLAY_WIDTH || dest_y >= DISPLAY_HEIGHT) {
                continue;
            }

            (*dest)[dest_x][dest_y].on = (*source_copy)[src_x][src_y].on;
            (*dest)[dest_x][dest_y].pwm = (*source_copy)[src_x][src_y].pwm;
        }
    }

    free(source_copy);
}

/**
 * Fill the entire display with a given pixel state.
 */
void display_fill(display_t* display, uint32_t pwm, bool on)
{
    display_rect(display, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, pwm, on);
}

/**
 * Fill the display with a checkerboard pattern.
 */
void display_checkerboard(display_t* display, bool invert, uint32_t pwm)
{
    bool state = invert;
    
    for (int x = 0; x < DISPLAY_WIDTH; x ++) {

        // If there are an even number of rows, invert after each row
        if (DISPLAY_WIDTH % 2 == 0) {
            state = !state;
        }

        for (int y = 0; y < DISPLAY_HEIGHT; y ++) {
            state = !state;
            (*display)[x][y].on = true;
            (*display)[x][y].pwm = state ? pwm : 0;
        }
    }
}