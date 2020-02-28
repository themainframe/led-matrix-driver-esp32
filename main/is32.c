#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "i2c.h"
#include "is32.h"

static const char* TAG = "IS32";
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

// Mutex protecting critical sections for the IS32 driver.
static portMUX_TYPE is32_mut = portMUX_INITIALIZER_UNLOCKED;

/**
 * Initialise the IS32 driver.
 */
void is32_init()
{
    i2c_init();
}

/**
 * Write a single-byte IS32 register.
 * Changes page automatically if required by the target register.
 */
bool is32_write_reg(is32_addr_t addr, uint16_t reg, uint8_t value)
{
    // If the register isn't global, ensure we're on the correct page first
    if (reg >> 8 != 0xFF) {
        is32_select_page(addr, (is32_page_t)(reg >> 8), true);
    }
    
    // Write the register
    bool result = true;
    portENTER_CRITICAL(&is32_mut);
    i2c_start();
    result |= i2c_tx(IS32_ADDRESS(addr) | I2C_WRITE_BIT);
    result |= i2c_tx(reg & 0xFF);
    result |= i2c_tx(value);
    i2c_stop();
    portEXIT_CRITICAL(&is32_mut);

    return result;
}

/**
 * Select the defined page on the IS32.
 * 
 * Handles the unlocking of the page selection register.
 * Optionally (use_cache) economises by remembering last-selected pages.
 */
void is32_select_page(is32_addr_t addr, is32_page_t page, bool use_cache)
{
    // Last-page cache initialised to invalid pages such that the first update always happens
    static uint8_t last_page_cache[IS32_CHIPS_PER_BUS] = {0xFF, 0xFF, 0xFF, 0xFF};

    // Is an update needed?
    if (last_page_cache[addr/5] == page) {
        return;
    }

    ESP_LOGD(TAG, "Updating page for %02x to %02x", addr, page);

    is32_write_reg(addr, IS32_REG_GLOBAL_UNLOCK, IS32_MAGIC_UNLOCK);
    is32_write_reg(addr, IS32_REG_GLOBAL_PAGE, (uint8_t)page);

    // Update the cache
    last_page_cache[addr/5] = page;
}
