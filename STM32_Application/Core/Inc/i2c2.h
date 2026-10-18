/*
 * i2c2.h
 *
 *  Created on: Feb 26, 2026
 *      Author: Rubin Khadka
 */

#ifndef I2C2_H_
#define I2C2_H_

#include "stdint.h"

typedef enum
{
  I2C_OK = 0,
  I2C_ERR_BUSY,
  I2C_ERR_START,
  I2C_ERR_NACK,
  I2C_ERR_TIMEOUT,
  I2C_ERR_BTF
} I2C_Status;

#define I2C_TIMEOUT   10000U

#define I2C_WRITE     0U
#define I2C_READ      1U

void I2C2_Init(void);
I2C_Status I2C2_Start(void);
void I2C2_Stop(void);
I2C_Status I2C2_SendAddr(uint8_t addr, uint8_t rw);
I2C_Status I2C2_WriteByte(uint8_t data);
I2C_Status I2C2_WriteByteWaitComplete(uint8_t data);
I2C_Status I2C2_ReadBurst(uint8_t devAddr, uint8_t regAddr, uint8_t *buf, uint8_t len);
I2C_Status I2C2_RepeatedStart(void);

#endif
