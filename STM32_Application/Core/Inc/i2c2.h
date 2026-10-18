/*
 * i2c2.h
 *
 *  Created on: Feb 26, 2026
 *      Author: Rubin Khadka
 */

#ifndef I2C2_H_
#define I2C2_H_

#include "stdint.h"

#define I2C_OK        0
#define I2C_ERROR     -1
#define I2C_TIMEOUT   -2

#define I2C_WRITE       0
#define I2C_READ        1

void    I2C2_Init(void);
uint8_t I2C2_Start(void);
void    I2C2_Stop(void);
uint8_t I2C2_SendAddr(uint8_t addr, uint8_t rw);
uint8_t I2C2_WriteByte(uint8_t data);
uint8_t I2C2_ReadByte(uint8_t ack);

/* NEW — needed by the rewritten i2c2.c */
uint8_t I2C2_RepeatedStart(void);
uint8_t I2C2_WriteByteWaitComplete(uint8_t data);
uint8_t I2C2_ReadBurst(uint8_t devAddr, uint8_t regAddr, uint8_t *buf, uint8_t len);

#endif /* I2C2_H_ */
