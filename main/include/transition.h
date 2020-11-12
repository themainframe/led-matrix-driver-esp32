#ifndef TRANSITION_H
#define TRANSITION_H

#include <stdio.h>
#include <stdbool.h>
#include "display.h"

/** Types **/

/**
 * Supported transition types.
 */
typedef enum {
    TRANS_WIPE,
    TRANS_FADE,
    TRANS_SCROLL_TEXT
} trans_type_t;

/** Wipe **/

// Wipe directions
#define TRANS_WIPE_DIRECTION_VERTICAL_MASK 0x01
typedef enum {
    WIPE_UP = 1,
    WIPE_RIGHT = 2,
    WIPE_DOWN = 3,
    WIPE_LEFT = 4
} trans_wipe_direction_t;

// Wipe state
typedef struct {
    trans_wipe_direction_t direction;
    unsigned int step;
} trans_wipe_data_t;

/** Fade **/

// Fade state
typedef struct {
    unsigned int steps;
    unsigned int step;
    float step_changes[DISPLAY_WIDTH][DISPLAY_HEIGHT];
} trans_fade_data_t;

/** Scroll Text **/

// Scroll start behaviours
typedef enum {

    // Start with the display empty
    SCROLL_START_CLEAR,

    // Start with the first part of the text already on the display
    SCROLL_START_FULL

} trans_scroll_text_start_behaviour_t;

// Scroll end behaviours
typedef enum {
    
    // End with the display clear
    SCROLL_END_CLEAR,

    // End with the end part of the text still on the display
    SCROLL_END_FULL

} trans_scroll_text_end_behaviour_t;

// Scroll Text state
typedef struct {
    bool invert;
    unsigned int step;
    const char* text;
    trans_scroll_text_start_behaviour_t start_behaviour;
    trans_scroll_text_end_behaviour_t end_behaviour;
} trans_scroll_text_data_t;

/** General types **/

/**
 * A handle that is used to progress a transition.
 * Pass the handle to trans_progress() to progress the transition.
 */
typedef struct trans_handle {
    
    // The original & destination display states.
    // `current` will be copied from the "from" state on transition start and updated as the transition progresses.
    // `to` will remain untouched.
    const display_t* to;
    display_t* current;
    
    // Is the transition finished?
    bool is_finished;

    // The transition type
    trans_type_t type;
    
    // Any data associated with the transition
    union {
        trans_wipe_data_t* wipe;
        trans_fade_data_t* fade;
        trans_scroll_text_data_t* scroll_text;
    } trans_data;

} trans_handle_t;

/** Procedures **/

// These functions create transitions
// Wipes upwards or downwards from one display state to another
trans_handle_t* trans_wipe(const display_t* from, const display_t* to, trans_wipe_direction_t direction);

// Fades from one display state to another in the defined number of steps
// The step for each pixel is calculated when trans_fade is called and each pixel is incremented on trans_progress()
trans_handle_t* trans_fade(const display_t* from, const display_t* to, unsigned int steps);

// Scroll a piece of text on the display
trans_handle_t* trans_scroll_text(const char* text, bool invert, trans_scroll_text_start_behaviour_t start, trans_scroll_text_end_behaviour_t end);

// Progresses a transition.
// Check is_finished on the trans_hand_t object to ascertain if the transition is complete.
trans_handle_t* trans_progress(trans_handle_t* handle);

// Free the resources used by a completed (or aborted) transition
// The new transition functions allocate trans_data with space to store any transition-specific progress data
// so it is critical that transition handles are freed after use.
void trans_free(trans_handle_t* handle);

#endif