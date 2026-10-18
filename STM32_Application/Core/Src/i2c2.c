/*
 * i2c2.c
 *
 *  Created on: Feb 26, 2026
 *      Author: Rubin Khadka
 *
 */

#include "stm32f103xb.h"
#include "i2c2.h"
#include <stddef.h>

#define I2C_LOOPS  10000U

/* ---------------------------------------------------------------- */
void I2C2_Init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
  RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

  /* PB10 SCL, PB11 SDA : AF open-drain, 2 MHz */
  GPIOB->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10 |
                  GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
  GPIOB->CRH |=  (GPIO_CRH_MODE10_1 | GPIO_CRH_CNF10_0 | GPIO_CRH_CNF10_1);
  GPIOB->CRH |=  (GPIO_CRH_MODE11_1 | GPIO_CRH_CNF11_0 | GPIO_CRH_CNF11_1);

  /* Soft-reset — PE must be 0 before SWRST */
  I2C2->CR1 &= ~I2C_CR1_PE;
  I2C2->CR1 |=  I2C_CR1_SWRST;
  for (volatile int i = 0; i < 100; i++);
  I2C2->CR1 &= ~I2C_CR1_SWRST;

  /* 100 kHz standard mode, PCLK1 = 36 MHz */
  I2C2->CR2   = 36U;
  I2C2->CCR   = 180U;   /* 36 MHz / (2 × 100 kHz) */
  I2C2->TRISE = 37U;

  I2C2->CR1 &= ~(I2C_CR1_ENGC | I2C_CR1_NOSTRETCH);
  I2C2->OAR1 = 0x4000;
  I2C2->CR1 |= I2C_CR1_PE | I2C_CR1_ACK;
}

/* ---------------------------------------------------------------- */
uint8_t I2C2_Start(void)
{
  uint32_t timeout = I2C_LOOPS;

  while (I2C2->SR2 & I2C_SR2_BUSY)
    if (--timeout == 0U) return I2C_ERROR;

  I2C2->CR1 |= I2C_CR1_START;

  timeout = I2C_LOOPS;
  while (!(I2C2->SR1 & I2C_SR1_SB))
    if (--timeout == 0U) return I2C_ERROR;

  return I2C_OK;
}

/* ---------------------------------------------------------------- */
uint8_t I2C2_RepeatedStart(void)
{
  uint32_t timeout = I2C_LOOPS;

  I2C2->CR1 |= I2C_CR1_START;

  while (!(I2C2->SR1 & I2C_SR1_SB))
    if (--timeout == 0U) return I2C_ERROR;

  return I2C_OK;
}

/* ---------------------------------------------------------------- */
void I2C2_Stop(void)
{
  I2C2->CR1 |= I2C_CR1_STOP;

  uint32_t timeout = I2C_LOOPS;
  while (I2C2->SR2 & I2C_SR2_BUSY)
    if (--timeout == 0U) break;
}

/* ---------------------------------------------------------------- */
uint8_t I2C2_SendAddr(uint8_t addr, uint8_t rw)
{
  uint32_t timeout = I2C_LOOPS;

  I2C2->SR1 &= ~I2C_SR1_AF;
  I2C2->DR   = (uint8_t)((addr << 1U) | (rw & 0x01U));

  while (!(I2C2->SR1 & I2C_SR1_ADDR))
  {
    if (I2C2->SR1 & I2C_SR1_AF)   { I2C2->CR1 |= I2C_CR1_STOP; return I2C_ERROR; }
    if (--timeout == 0U)          { I2C2->CR1 |= I2C_CR1_STOP; return I2C_TIMEOUT; }
  }

  (void) I2C2->SR1;   /* ADDR-clear sequence: read SR1 */
  (void) I2C2->SR2;   /*                     then SR2 */
  return I2C_OK;
}

/* ---------------------------------------------------------------- */
uint8_t I2C2_WriteByte(uint8_t data)
{
  uint32_t timeout = I2C_LOOPS;

  while (!(I2C2->SR1 & I2C_SR1_TXE))
  {
    if (I2C2->SR1 & I2C_SR1_AF) { I2C2->CR1 |= I2C_CR1_STOP; return I2C_ERROR; }
    if (--timeout == 0U)        { I2C2->CR1 |= I2C_CR1_STOP; return I2C_TIMEOUT; }
  }

  I2C2->DR = data;
  return I2C_OK;
}

/* ---------------------------------------------------------------- */
uint8_t I2C2_WriteByteWaitComplete(uint8_t data)
{
  uint8_t st = I2C2_WriteByte(data);
  if (st != I2C_OK) return st;

  uint32_t timeout = I2C_LOOPS;
  while (!(I2C2->SR1 & I2C_SR1_BTF))
  {
    if (I2C2->SR1 & I2C_SR1_AF) { I2C2->CR1 |= I2C_CR1_STOP; return I2C_ERROR; }
    if (--timeout == 0U)        { I2C2->CR1 |= I2C_CR1_STOP; return I2C_TIMEOUT; }
  }
  return I2C_OK;
}

/* ---------------------------------------------------------------- */
static void I2C2_Abort(void)
{
  I2C2->CR1 &= ~I2C_CR1_POS;
  I2C2->CR1 |=  I2C_CR1_STOP;

  uint32_t timeout = I2C_LOOPS;
  while ((I2C2->SR2 & I2C_SR2_BUSY) && --timeout) { }
}

/* ---------------------------------------------------------------- */
uint8_t I2C2_ReadBurst(uint8_t devAddr, uint8_t regAddr,
                       uint8_t *buf, uint8_t len)
{
  uint8_t  st;
  uint32_t timeout;

  if (len == 0U || buf == NULL) return I2C_ERROR;

  /* Register pointer */
  st = I2C2_Start();                          if (st) { I2C2_Abort(); return st; }
  st = I2C2_SendAddr(devAddr, I2C_WRITE);     if (st) { I2C2_Abort(); return st; }
  st = I2C2_WriteByteWaitComplete(regAddr);   if (st) { I2C2_Abort(); return st; }

  /* Repeated START + read address */
  st = I2C2_RepeatedStart();                  if (st) { I2C2_Abort(); return st; }
  st = I2C2_SendAddr(devAddr, I2C_READ);      if (st) { I2C2_Abort(); return st; }

  if (len == 1U)
  {
    /* NACK + STOP before the byte arrives */
    I2C2->CR1 &= ~I2C_CR1_ACK;
    I2C2->CR1 |=  I2C_CR1_STOP;

    timeout = I2C_LOOPS;
    while (!(I2C2->SR1 & I2C_SR1_RXNE))
      if (--timeout == 0U) { I2C2_Abort(); return I2C_TIMEOUT; }

    buf[0] = (uint8_t) I2C2->DR;
  }
  else if (len == 2U)
  {
    /* POS = 1 : NACK byte 1, STOP after byte 2 */
    I2C2->CR1 |=  I2C_CR1_POS;
    I2C2->CR1 &= ~I2C_CR1_ACK;

    timeout = I2C_LOOPS;
    while (!(I2C2->SR1 & I2C_SR1_BTF))
      if (--timeout == 0U) { I2C2_Abort(); return I2C_TIMEOUT; }

    I2C2->CR1 |= I2C_CR1_STOP;
    buf[0] = (uint8_t) I2C2->DR;
    buf[1] = (uint8_t) I2C2->DR;
    I2C2->CR1 &= ~I2C_CR1_POS;
  }
  else
  {
    uint8_t i;

    for (i = 0U; i < (uint8_t)(len - 3U); i++)
    {
      timeout = I2C_LOOPS;
      while (!(I2C2->SR1 & I2C_SR1_RXNE))
        if (--timeout == 0U) { I2C2_Abort(); return I2C_TIMEOUT; }
      buf[i] = (uint8_t) I2C2->DR;
    }

    /* BTF: byte N-2 in DR, byte N-1 in shift register */
    timeout = I2C_LOOPS;
    while (!(I2C2->SR1 & I2C_SR1_BTF))
      if (--timeout == 0U) { I2C2_Abort(); return I2C_TIMEOUT; }

    I2C2->CR1 &= ~I2C_CR1_ACK;
    buf[len - 3U] = (uint8_t) I2C2->DR;

    timeout = I2C_LOOPS;
    while (!(I2C2->SR1 & I2C_SR1_RXNE))
      if (--timeout == 0U) { I2C2_Abort(); return I2C_TIMEOUT; }
    buf[len - 2U] = (uint8_t) I2C2->DR;

    I2C2->CR1 |= I2C_CR1_STOP;

    timeout = I2C_LOOPS;
    while (!(I2C2->SR1 & I2C_SR1_RXNE))
      if (--timeout == 0U) { I2C2_Abort(); return I2C_TIMEOUT; }
    buf[len - 1U] = (uint8_t) I2C2->DR;
  }

  return I2C_OK;
}

/* ---------------------------------------------------------------- */
uint8_t I2C2_ReadByte(uint8_t ack)
{
  uint32_t timeout = I2C_LOOPS;

  if (ack)
  {
    I2C2->CR1 |= I2C_CR1_ACK;
  }
  else
  {
    I2C2->CR1 &= ~I2C_CR1_ACK;
    I2C2->CR1 |=  I2C_CR1_STOP;
  }

  while (!(I2C2->SR1 & I2C_SR1_RXNE))
    if (--timeout == 0U) return 0xFF;

  return (uint8_t) I2C2->DR;
}
