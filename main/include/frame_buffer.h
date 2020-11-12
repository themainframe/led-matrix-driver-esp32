//
// A frame buffer that keeps display updates synchronised.
//

#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "display.h"

typedef struct {
  display_t frame;
  SemaphoreHandle_t sem;
  bool dirty;
} frame_buffer_t;

// Initialise the framebuffer
void fb_init();

// Push a new frame
void fb_push(display_t* new);

// Write the current frame to the display
bool fb_write();

#endif