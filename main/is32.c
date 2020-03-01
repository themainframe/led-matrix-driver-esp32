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
 * Sequential-write to an IS32.
 * Changes page automatically if required by the target register.
 */
bool is32_write_seq(is32_addr_t addr, uint16_t start_reg, const uint8_t *data, uint length)
{
    // The start register should never be global
    // No IS32 operations involve seq writes to global registers
    if (start_reg >> 8 == 0xFF) {
        ESP_LOGW(TAG, "Sequential write to global register %02x not supported", start_reg);
        return false;
    }

    // Change page if required
    bool result = true;
    result &= is32_select_page(addr, (is32_page_t)(start_reg >> 8), true);
    if (!result) {
        return result;
    }

    // Write the registers
    portENTER_CRITICAL(&is32_mut);
    i2c_start();
    
    // Write address and register
    result &= i2c_tx(IS32_ADDRESS(addr) | I2C_WRITE_BIT);
    result &= i2c_tx(start_reg & 0xFF);
    
    // Write each byte
    for (;length && result; length --) {
        // Transmit a byte, then increment to the next one - anding the result to ensure it didn't fail
        result &= i2c_tx(*data ++);
    }

    i2c_stop();
    portEXIT_CRITICAL(&is32_mut);

    return result;
}

/**
 * Write a single-byte IS32 register.
 * Changes page automatically if required by the target register.
 */
bool is32_write_reg(is32_addr_t addr, uint16_t reg, uint8_t value)
{
    bool result = true;

    // If the register isn't global, ensure we're on the correct page first
    if (reg >> 8 != 0xFF) {
        result &= is32_select_page(addr, (is32_page_t)(reg >> 8), true);
        if (!result) {
            return result;
        }
    }
    
    // Write the register
    portENTER_CRITICAL(&is32_mut);
    i2c_start();
    result &= i2c_tx(IS32_ADDRESS(addr) | I2C_WRITE_BIT);
    result &= i2c_tx(reg & 0xFF);
    result &= i2c_tx(value);
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
bool is32_select_page(is32_addr_t addr, is32_page_t page, bool use_cache)
{
    // Last-page cache initialised to invalid pages such that the first update always happens
    static uint8_t last_page_cache[IS32_CHIPS_PER_BUS] = {0xFF, 0xFF, 0xFF, 0xFF};

    // Is an update needed?
    if (last_page_cache[addr/5] == page) {
        return true;
    }

    bool result = true;
    result &= is32_write_reg(addr, IS32_REG_GLOBAL_UNLOCK, IS32_MAGIC_UNLOCK);
    result &= is32_write_reg(addr, IS32_REG_GLOBAL_PAGE, (uint8_t)page);

    // Update the cache
    last_page_cache[addr/5] = page;
    return result;
}
