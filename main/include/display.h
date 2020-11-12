#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

// Define the width and height of the full display
#define DISPLAY_WIDTH 24
#define DISPLAY_HEIGHT 6

// Define the maximum area covered by each chip
#define IS32_WIDTH 8
#define IS32_HEIGHT 6

// Number of chips needed to cover the width
#define IS32_CHIPS_WIDE (((DISPLAY_WIDTH - 1) / IS32_WIDTH) + 1)

// Number of chips needed to cover the height
#define IS32_CHIPS_HIGH (((DISPLAY_HEIGHT - 1) / IS32_HEIGHT) + 1)

// Define the number of chips used to cover the whole display
#define IS32_CHIPS (IS32_CHIPS_WIDE > IS32_CHIPS_HIGH ? IS32_CHIPS_WIDE : IS32_CHIPS_HIGH)

// Define some macros to help convert positions on the whole display to chip-relative positions
#define IS32_FIRST_COL(chip) (chip * IS32_WIDTH)
#define IS32_LAST_COL(chip) (((chip + 1) * IS32_WIDTH))

// Define the dimensions of characters (including spaces between them)
#define DISPLAY_CHAR_WIDTH 5
#define DISPLAY_CHAR_HEIGHT 5

// Define a type for the state of a single LED
typedef struct {

    // The PWM value
    // It isn't entirely clear from the IS32 datasheet how exactly this works...
    uint32_t pwm;

    // Whether the LED should be ON or OFF
    bool on;
    
    // Flags that indicate shorted/open-circuit LEDs
    bool is_shorted;
    bool is_open;

} led_t;

// Define types that represent rows and columns of the display
typedef led_t column_t[DISPLAY_HEIGHT];
typedef column_t display_t[DISPLAY_WIDTH];

// Procs
void display_init();
void display_update(display_t* display);
void display_fill(display_t* display, uint32_t pwm, bool on);
void display_checkerboard(display_t* display, bool invert, uint32_t pwm);
void display_text(display_t* display, int x_pos, uint32_t pwm, const char* text);
void display_rect(display_t* display, int x_pos, int y_pos, int width, int height, uint32_t pwm, bool on);
void display_copy(const display_t* source, display_t* dest, int src_x_pos, int src_y_pos, int dest_x_pos, int dest_y_pos, int width, int height);

#endif