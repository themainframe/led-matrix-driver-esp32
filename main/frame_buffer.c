#include <string.h>
#include "display.h"
#include "frame_buffer.h"
#include "esp_log.h"

frame_buffer_t frame_buffer;
static const char* TAG = "FB";

/**
 * Initialise the framebuffer.
 */
void fb_init()
{
    frame_buffer.dirty = true;
    frame_buffer.sem = xSemaphoreCreateBinary();
    xSemaphoreGive(frame_buffer.sem);
}

/**
 * Push a new frame.
 */
void fb_push(display_t* new)
{
    // Acquire the semaphore for the frame buffer
    // if we can't update the current frame, just drop this one and try again next time
    if (xSemaphoreTake(frame_buffer.sem, 0) == pdTRUE) {

        // Copy the frame into place
        memcpy(frame_buffer.frame, new, sizeof(display_t));

        // Mark the frame as dirty
        frame_buffer.dirty = true;

        xSemaphoreGive(frame_buffer.sem);

    } else {
        ESP_LOGW(TAG, "couldn't obtain framebuffer sem, dropping frame");
    }
}

/**
 * Write the current frame.
 * Returns true if the display needed an update, or false otherwise.
 */
bool fb_write()
{
    // Acquire the semaphore and write the display state
    if (xSemaphoreTake(frame_buffer.sem, 25 / portTICK_PERIOD_MS) == pdTRUE) {

        if (!frame_buffer.dirty) {
            xSemaphoreGive(frame_buffer.sem);
            return false;
        }

        display_update(&(frame_buffer.frame));
        frame_buffer.dirty = false;
        xSemaphoreGive(frame_buffer.sem);
    }
    
    return true;
}
