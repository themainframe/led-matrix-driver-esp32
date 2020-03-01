#ifndef IS32_H
#define IS32_H

// Make an actual I2C address from an is32_addr_t
#define IS32_ADDRESS(a) ((0x50 | a) << 1)

// Chips per I2C bus
#define IS32_CHIPS_PER_BUS 4

// For IS32_REG_* - MSB contains the page, LSB contains the Register
// FF in the MSB indicates a global register

// Global Registers
#define IS32_REG_GLOBAL_PAGE 0xFFFD
#define IS32_REG_GLOBAL_UNLOCK 0xFFFE

// Value that when written to IS32_REG_GLOBAL_UNLOCK unlocks the page select register
#define IS32_MAGIC_UNLOCK 0xC5

// Page-specific Registers
#define IS32_REG_CONFIG 0x0300
#define IS32_REG_GLOBAL_CURRENT_CONTROL 0x0301

// Matrix control registers
#define IS32_REG_LED_ON_OFF_START 0x0000
#define IS32_REG_LED_ON_OFF_END 0x0017
#define IS32_REG_PWM_START 0x0100
#define IS32_REG_PWM_END 0x01BF

// IS32 Pages
typedef enum {
    IS32_PAGE_LED_CTRL = 0,
    IS32_PAGE_PWM = 1,
    IS32_PAGE_ABM = 2,
    IS32_PAGE_FUNC = 3
} is32_page_t;

// IS32 Addresses on a single I2C Bus
// NOTE: These are specifically assigned according to the TXLED board
typedef enum {
    IS32_ADDRESS_A = 0,
    IS32_ADDRESS_B = 15,
    IS32_ADDRESS_C = 10,
    IS32_ADDRESS_D = 5
} is32_addr_t;

// -- Configuration register enums

// SYNC pin behaviour
typedef enum {
    IS32_SYNC_HIGH_Z = 0b0,
    IS32_SYNC_MASTER = 0b01000000,
    IS32_SYNC_SLAVE =  0b10000000
} is32_config_sync_t;

// Open/Short Detect behaviour
typedef enum {
    IS32_OSD_DONT_TRIGGER = 0b0,
    IS32_OSD_TRIGGER_NOW =  0b00000100
} is32_config_osd_t;

// ABM Start trigger
typedef enum {
    IS32_ABM_DONT_TRIGGER = 0b0,
    IS32_ABM_TRIGGER_NOW = 0b00000010
} is32_config_abm_t;

// Software shutdown
typedef enum {
    IS32_SSD_SHUTDOWN = 0b0,
    IS32_SSD_RUN = 0b00000001
} is32_config_ssd_t;

// Procedures
void is32_init();
bool is32_select_page(is32_addr_t addr, is32_page_t page, bool use_cache);
bool is32_write_reg(is32_addr_t addr, uint16_t reg, uint8_t value);
bool is32_write_seq(is32_addr_t addr, uint16_t start_reg, const uint8_t *data, uint length);

#endif