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

// Define the number of chips used to cover the whole display
#define IS32_CHIPS MAX((DISPLAY_WIDTH/IS32_WIDTH), (DISPLAY_HEIGHT/IS32_HEIGHT))

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

#endif