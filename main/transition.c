#include <stdio.h>
#include <string.h>
#include "transition.h"
#include "esp_log.h"

/**
 * Performs transitions between display states.
 *
 * Typical example of the use of a transition:
 * 
 * trans_handle_t* wipe = trans_wipe_start(current, new, WIPE_UP, NULL);
 * while(!wipe->is_finished) {
 *     trans_progress(wipe);
 *     // Update the display here to be wipe->current
 * }
 * trans_free(wipe);
 */

static const char* TAG = "Trans";
// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

/**
 * Wipe from one display state to another vertically.
 */
trans_handle_t* trans_wipe(const display_t* from, const display_t* to, trans_wipe_direction_t direction)
{
    // Allocate space for the handle
    trans_handle_t* handle = malloc(sizeof(trans_handle_t));
    handle->current = malloc(sizeof(display_t));
    handle->to = to;
    handle->is_finished = false;

    // Copy the starting state
    display_copy(from, handle->current, 0, 0, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Allocate space for the progress data
    handle->type = TRANS_WIPE;
    handle->trans_data.wipe = malloc(sizeof(trans_wipe_data_t));
    handle->trans_data.wipe->direction = direction;
    handle->trans_data.wipe->step = 0;
    return handle;
}

/**
 * Fade from one display state to another.
 */
trans_handle_t* trans_fade(const display_t* from, const display_t* to, unsigned int steps)
{
    // Allocate space for the handle
    trans_handle_t* handle = malloc(sizeof(trans_handle_t));
    handle->current = malloc(sizeof(display_t));
    handle->to = to;
    handle->is_finished = false;

    // Copy the starting state
    display_copy(from, handle->current, 0, 0, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Allocate space for the progress data
    handle->type = TRANS_FADE;
    handle->trans_data.fade = malloc(sizeof(trans_fade_data_t));
    handle->trans_data.fade->step = 0;
    handle->trans_data.fade->steps = steps;

    // For each pixel, calculate the step change
    for (uint x = 0; x < DISPLAY_WIDTH; x ++) {
        for (uint y = 0; y < DISPLAY_HEIGHT; y ++) {
            // Calculate a floating point intensity change per step of the transition for this pixel
            int intensity_diff = (int)(*from)[x][y].pwm - (int)(*to)[x][y].pwm;
            handle->trans_data.fade->step_changes[x][y] = (float)intensity_diff / (float)steps;
            ESP_LOGD(TAG, "%d/%d intens. total_diff = %d, chg = %02f", x, y, intensity_diff, handle->trans_data.fade->step_changes[x][y]);
        }
    }

    return handle;
}

/**
 * Scroll text horizontally on the display.
 */
trans_handle_t* trans_scroll_text(const char* text, bool invert, trans_scroll_text_start_behaviour_t start, trans_scroll_text_end_behaviour_t end)
{
    // Allocate space for the handle
    trans_handle_t* handle = malloc(sizeof(trans_handle_t));
    handle->current = malloc(sizeof(display_t));
    handle->is_finished = false;

    // Allocate space for the progress data
    handle->type = TRANS_SCROLL_TEXT;
    handle->trans_data.scroll_text = malloc(sizeof(trans_scroll_text_data_t));
    handle->trans_data.scroll_text->step = 0;
    handle->trans_data.scroll_text->invert = invert;
    handle->trans_data.scroll_text->text = text;
    handle->trans_data.scroll_text->start_behaviour = start;
    handle->trans_data.scroll_text->end_behaviour = end;
    return handle;
}

/**
 * Progress the wipe transition.
 */
trans_handle_t* trans_wipe_progress(trans_handle_t* handle)
{
    // Vertical or horizontal wiping?
    if (handle->trans_data.wipe->direction & TRANS_WIPE_DIRECTION_VERTICAL_MASK) {

        ESP_LOGI(TAG, "trans_wipe: step %d", handle->trans_data.wipe->step);

        // Copy everything currently in view up or down one line
        display_copy(
            handle->current, // From
            handle->current, // To
            0, 0, // From 0, 0
            0, handle->trans_data.wipe->direction == WIPE_UP ? -1 : 1, // Move everything one line up or down
            DISPLAY_WIDTH, DISPLAY_HEIGHT // Move the whole viewport
        );

        // Replace the bottom or line with the relevant line from the new view
        display_copy(
            handle->to, // From
            handle->current, // To

            // From either lines working from the top or bottom of the transition target
            0, handle->trans_data.wipe->direction == WIPE_UP ? (handle->trans_data.wipe->step) : (DISPLAY_HEIGHT - handle->trans_data.wipe->step),
            
            // To either the bottom or top line
            0, handle->trans_data.wipe->direction == WIPE_UP ? (DISPLAY_HEIGHT - 1) : 0,
            
            // Always one line, the full width of the display
            DISPLAY_WIDTH, 1
        );

        // Done once we've wiped the entire height of the display
        if ((handle->trans_data.wipe->step)++ == DISPLAY_HEIGHT) {
            handle->is_finished = true;
            ESP_LOGI(
                TAG, "trans_wipe (@ %p) has finished (%d steps)",
                handle, handle->trans_data.fade->step
            );
        }
    }

    return handle;
}

/**
 * Progress the fade transition.
 */
trans_handle_t* trans_fade_progress(trans_handle_t* handle)
{
    ESP_LOGD(TAG, "progressing fade trans @ %p step %d", handle, handle->trans_data.fade->step);

    // Apply the step change to each pixel
    for (uint x = 0; x < DISPLAY_WIDTH; x ++) {
        for (uint y = 0; y < DISPLAY_HEIGHT; y ++) {
            unsigned int new_pwm = (unsigned int)(((float)((*handle->current)[x][y].pwm) - handle->trans_data.fade->step_changes[x][y] ) + 0.5);
            (*handle->current)[x][y].pwm = new_pwm;
            ESP_LOGD(TAG, "update px %d/%d chg %02f now %d", x, y, handle->trans_data.fade->step_changes[x][y], new_pwm);
        }
    }

    if (++(handle->trans_data.fade->step) == handle->trans_data.fade->steps) {
        
        handle->is_finished = true;

        // Finalise the transition, making up for any rounding errors
        display_copy(handle->to, handle->current, 0, 0, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

        ESP_LOGI(
            TAG, "trans_fade (@ %p) has finished (%d steps)",
            handle, handle->trans_data.fade->steps
        );
    }

    return handle;
}

/**
 * Progress the scroll text transition.
 */
trans_handle_t* trans_scroll_text_progress(trans_handle_t* handle)
{
    // Start with a blank canvas each time, and draw the text at the appropriate offset based on step
    int text_pixel_len = strlen(handle->trans_data.scroll_text->text) * DISPLAY_CHAR_WIDTH;

    // Clear the display & write text
    display_fill(handle->current, handle->trans_data.scroll_text->invert ? 0xff : 0x00, true);
    display_text(
        handle->current,
        (handle->trans_data.scroll_text->start_behaviour == SCROLL_START_CLEAR ? DISPLAY_WIDTH : 0) - handle->trans_data.scroll_text->step,
        handle->trans_data.scroll_text->invert ? 0x00 : 0xff,
        handle->trans_data.scroll_text->text
    );

    // Finished when the rightmost pixel of text is < 0
    if (
        (int)text_pixel_len - handle->trans_data.scroll_text->step + (handle->trans_data.scroll_text->start_behaviour == SCROLL_START_CLEAR ? DISPLAY_WIDTH : 0) <
        (handle->trans_data.scroll_text->end_behaviour == SCROLL_END_CLEAR ? 0 : DISPLAY_WIDTH)
    ) {
        handle->is_finished = true;
        ESP_LOGI(
            TAG, "trans_scroll_text (@ %p) has finished (%d steps)",
            handle, handle->trans_data.scroll_text->step
        );
    }
    
    handle->trans_data.scroll_text->step ++;
    return handle;
}

/**
 * Progress a transition.
 */
trans_handle_t* trans_progress(trans_handle_t* handle)
{
    if (handle == NULL || handle->is_finished) {
        ESP_LOGW(TAG, "attempted to progress a completed/null transition - ignored");
        return handle;
    }

    // Call the appropriate transition progress method
    switch (handle->type) {
        case TRANS_WIPE: return trans_wipe_progress(handle);
        case TRANS_FADE: return trans_fade_progress(handle);
        case TRANS_SCROLL_TEXT: return trans_scroll_text_progress(handle);
        default: ESP_LOGW(TAG, "unknown transition type: %d", handle->type);
    }

    return handle;
}

/**
 * Free the transition handle.
 */
void trans_free(trans_handle_t* handle)
{
    if (handle == NULL) {
        return;
    }

    // Free the copied "from" display buffer
    if (handle->current != NULL) {
        free(handle->current);
    }
    
    // Free the transition data
    switch (handle->type) {
        case TRANS_WIPE: free(handle->trans_data.wipe); break;
        case TRANS_FADE: free(handle->trans_data.fade); break;
        case TRANS_SCROLL_TEXT: free(handle->trans_data.scroll_text); break;
        default: ESP_LOGW(TAG, "not freeing unknown transition type: %d", handle->type);
    }

    // Free the handle itself
    free(handle);
}
