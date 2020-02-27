#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include "pins.h"

typedef enum {
    BUTTON_A = PIN_BUTTON_A,
    BUTTON_B = PIN_BUTTON_B
} button_t;

// Methods
void buttons_init();
bool button_is_pressed(button_t button);

#endif