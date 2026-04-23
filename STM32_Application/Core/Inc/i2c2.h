/*
 * i2c2.h
 *
 *  Created on: Feb 26, 2026
 *      Author: Rubin Khadka
 */

#ifndef I2C2_H_
#define I2C2_H_

#include "stdint.h"

// I2C Status codes
#define I2C_OK        0
#define I2C_ERROR     -1
#define I2C_TIMEOUT   -2

// I2C Read/Write flags
#define I2C_WRITE       0
#define I2C_READ        1

// Function Prototypes
void I2C2_Init(void);
void I2C2_Start(void);
void I2C2_Stop(void);
uint8_t I2C2_SendAddr(uint8_t addr, uint8_t rw);
uint8_t I2C2_WriteByte(uint8_t data);
uint8_t I2C2_ReadByte(uint8_t ack);

#endif /* I2C2_H_ */
