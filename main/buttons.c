#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "pins.h"
#include "buttons.h"

/**
 * Initialise the button controls.
 */
void buttons_init()
{
    gpio_pad_select_gpio(PIN_BUTTON_A);
    gpio_set_direction(PIN_BUTTON_A, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BUTTON_A, GPIO_PULLUP_ONLY);
    gpio_pad_select_gpio(PIN_BUTTON_B);
    gpio_set_direction(PIN_BUTTON_B, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BUTTON_B, GPIO_PULLUP_ONLY);
}

/**
 * Returns whether or not the button is currently pressed.
 */
bool button_is_pressed(button_t button)
{
    return gpio_get_level((gpio_num_t)button) == 0;
}
