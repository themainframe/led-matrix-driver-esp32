#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// Cycle count delay for software I2C interface
#define I2C_WAIT_CYCLES 25

// Write/read bit value
#define I2C_WRITE_BIT 0x00
#define I2C_READ_BIT 0x01

// Procedures
void i2c_init();
void i2c_start();
void i2c_stop();
uint8_t i2c_rx(bool send_ack);
bool i2c_tx(uint8_t data);


#endif