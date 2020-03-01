#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

// Define the width and height of the full display
#define DISPLAY_WIDTH 20
#define DISPLAY_HEIGHT 5

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
void display_text(display_t* display, uint x_pos, uint32_t pwm, const char* text);

#endif