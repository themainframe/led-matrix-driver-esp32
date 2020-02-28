#include <stdio.h>
#include "esp_log.h"
#include "xtensa/core-macros.h"
#include "freertos/FreeRTOS.h"
#include "freeRTOS/portmacro.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "pins.h"
#include "i2c.h"
           
static const char* TAG = "I2C";

/**
 * Set up the I2C GPIO pads.
 */
void i2c_init()
{
  // Set up the I2C peripheral
  ESP_LOGI(TAG, "setting up GPIO pins");
  gpio_set_level(PIN_SDA, 1);
  gpio_set_level(PIN_SCL, 1);
  gpio_set_direction(PIN_SCL, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_set_direction(PIN_SDA, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_set_pull_mode(PIN_SCL, GPIO_FLOATING);
  gpio_set_pull_mode(PIN_SDA, GPIO_FLOATING);
}

/**
 * Insert a wait.
 */
void wait()
{
  uint32_t c_current = 0, c_start = XTHAL_GET_CCOUNT();
  do {
    c_current = XTHAL_GET_CCOUNT();
  } while (c_current - c_start < I2C_WAIT_CYCLES);
}

static void scl_hi()
{
  gpio_set_level(PIN_SCL, 1);
  wait(1);
}

static void scl_lo()
{
  gpio_set_level(PIN_SCL, 0);
  wait(1);
}

static void sda_hi()
{
  gpio_set_level(PIN_SDA, 1);
  wait(1);
}

static void sda_lo()
{
  gpio_set_level(PIN_SDA, 0);
  wait(1);
}

static bool sda_read()
{
  return gpio_get_level(PIN_SDA) != 0;
}

/**
 * Send a start bit sequence.
 */
void i2c_start()
{
  sda_hi();
  scl_hi();
  sda_lo();
  scl_lo();
  wait(1);
}

/**
 * Send a stop bit sequence.
 */
void i2c_stop()
{
  sda_lo();
  scl_hi();
  sda_hi();
  wait(1);
}

/**
 * Transmit a single byte on the I2C interface.
 */
bool i2c_tx(uint8_t data)
{
  // Shift out `data`
  for (uint x = 8; x; x--) {
    if (data & 0x80) {
      sda_hi();
    } else {
      sda_lo();
    }
    scl_hi();
    data <<= 1;
    scl_lo();
  }

  // Read the acknowledgement (if present)
  sda_hi();
  scl_hi();
  bool ret = sda_read() ? false : true;
  scl_lo();

  return ret;
}

/**
 * Receive a single byte on the I2C interface, optionally with an acknowledgement sent
 */
uint8_t i2c_rx(bool send_ack)
{
  uint8_t data = 0;
  sda_hi();

  for (uint x = 0; x < 8; x ++) {

    // Shift into `data`
    data <<= 1;

    scl_hi();
    wait(1);
    if (sda_read()) {
      data |= 1;
    }
    scl_lo();

    // Should we acknowledge?
    if (send_ack) {
      sda_lo();
    } else {
      sda_hi();
    }
  }

  scl_hi();
  scl_lo();
  sda_hi();

  return data;
}
