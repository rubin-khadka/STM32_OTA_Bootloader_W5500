/*
 * i2c2.c
 *
 *  Created on: Feb 26, 2026
 *      Author: Rubin Khadka
 *
 */

#include "i2c2.h"
#include "main.h"

void I2C2_Init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
  RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

  /* PB10 SCL, PB11 SDA : AF open drain, 2 MHz */
  GPIOB->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10 |
  GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
  GPIOB->CRH |= (GPIO_CRH_MODE10_1 | GPIO_CRH_CNF10_0 | GPIO_CRH_CNF10_1);
  GPIOB->CRH |= (GPIO_CRH_MODE11_1 | GPIO_CRH_CNF11_0 | GPIO_CRH_CNF11_1);

  I2C2->CR1 &= ~I2C_CR1_PE;
  I2C2->CR1 |= I2C_CR1_SWRST;
  for(volatile int i = 0; i < 100; i++);
  I2C2->CR1 &= ~I2C_CR1_SWRST;

  I2C2->CR2 = 36U; /* 36 MHz */
  I2C2->CCR = 180U; /* 100 kHz */
  I2C2->TRISE = 37U; /* 36 + 1 */

  // Set duty cycle
  I2C2->CCR &= ~I2C_CCR_DUTY;

  // Disable general call, no stretch, dual address disable
  I2C2->CR1 &= ~(I2C_CR1_ENGC | I2C_CR1_NOSTRETCH);

  // Set own address to 0
  I2C2->OAR1 = 0x4000;

  I2C2->CR1 = I2C_CR1_ACK;
  I2C2->CR1 |= I2C_CR1_PE;
}

// Start Condition
I2C_Status I2C2_Start(void)
{
  uint32_t timeout = I2C_TIMEOUT;

  /* Wait until bus is free */
  while(I2C2->SR2 & I2C_SR2_BUSY)
  {
    if(--timeout == 0U)
    {
      return I2C_ERR_BUSY;
    }
  }

  /* Generate START */
  I2C2->CR1 |= I2C_CR1_START;

  /* Wait for Start Bit */
  timeout = I2C_TIMEOUT;
  while(!(I2C2->SR1 & I2C_SR1_SB))
  {
    if(--timeout == 0U)
    {
      return I2C_ERR_START;
    }
  }
  return I2C_OK;
}

// Stop Condition
void I2C2_Stop(void)
{
  I2C2->CR1 |= I2C_CR1_STOP;

  /* Wait for bus to go idle */
  uint32_t timeout = I2C_TIMEOUT;
  while(I2C2->SR2 & I2C_SR2_BUSY)
  {
    if(--timeout == 0U)
    {
      break;
    }
  }
}

// Send I2C Address
I2C_Status I2C2_SendAddr(uint8_t addr, uint8_t rw)
{
  uint32_t timeout = I2C_TIMEOUT;

  /* Clear pending ACK failure flag */
  I2C2->SR1 &= ~I2C_SR1_AF;

  I2C2->DR = (uint8_t) ((addr << 1U) | (rw & 0x01U));

  /* Wait for ADDR */
  while(!(I2C2->SR1 & I2C_SR1_ADDR))
  {
    if(I2C2->SR1 & I2C_SR1_AF)
    {
      I2C2->CR1 |= I2C_CR1_STOP; // Release Bus
      return I2C_ERR_NACK;
    }
    if(--timeout == 0U)
    {
      I2C2->CR1 |= I2C_CR1_STOP; // Release Bus
      return I2C_ERR_TIMEOUT;
    }
  }

  /* Clear ADDR: read SR1 then SR2 */
  (void) I2C2->SR1;
  (void) I2C2->SR2;

  return I2C_OK;
}

// Write Single Byte
I2C_Status I2C2_WriteByte(uint8_t data)
{
  uint32_t timeout = I2C_TIMEOUT;

  while(!(I2C2->SR1 & I2C_SR1_TXE))
  {
    if(I2C2->SR1 & I2C_SR1_AF)
    {
      I2C2->CR1 |= I2C_CR1_STOP;
      return I2C_ERR_NACK;
    }
    if(--timeout == 0U)
    {
      I2C2->CR1 |= I2C_CR1_STOP;
      return I2C_ERR_TIMEOUT;
    }
  }

  I2C2->DR = data;
  return I2C_OK;
}

// Write Byte and Wait
I2C_Status I2C2_WriteByteWaitComplete(uint8_t data)
{
  I2C_Status status = I2C2_WriteByte(data);
  if(status != I2C_OK)
  {
    return status;
  }

  uint32_t timeout = I2C_TIMEOUT;
  while(!(I2C2->SR1 & I2C_SR1_BTF))
  {
    if(I2C2->SR1 & I2C_SR1_AF)
    {
      I2C2->CR1 |= I2C_CR1_STOP;
      return I2C_ERR_NACK;
    }
    if(--timeout == 0U)
    {
      I2C2->CR1 |= I2C_CR1_STOP;
      return I2C_ERR_TIMEOUT;
    }
  }
  return I2C_OK;
}

I2C_Status I2C2_ReadBurst(uint8_t devAddr, uint8_t regAddr, uint8_t *buf, uint8_t len)
{
  I2C_Status status;
  uint32_t timeout;

  if(len == 0U || buf == NULL)
  {
    return I2C_ERR_TIMEOUT;
  }

  /* Write register pointer */
  status = I2C2_Start();
  if(status != I2C_OK)
    return status;

  status = I2C2_SendAddr(devAddr, 0U);
  if(status != I2C_OK)
    return status;

  status = I2C2_WriteByteWaitComplete(regAddr);
  if(status != I2C_OK)
    return status;

  /* Repeated START + Read */
  status = I2C2_RepeatedStart();
  if(status != I2C_OK)
    return status;

  status = I2C2_SendAddr(devAddr, 1U);
  if(status != I2C_OK)
    return status;

  /* Enable ACK for multi‑byte reads */
  if(len > 1U)
  {
    I2C2->CR1 |= I2C_CR1_ACK;
  }

  /* Receive bytes */
  if(len == 1U)
  {
    I2C2->CR1 &= ~I2C_CR1_ACK;
    I2C2->CR1 |= I2C_CR1_STOP;

    timeout = I2C_TIMEOUT;
    while(!(I2C2->SR1 & I2C_SR1_RXNE))
    {
      if(--timeout == 0U)
        return I2C_ERR_TIMEOUT;
    }
    buf[0] = (uint8_t) I2C2->DR;

  }
  else if(len == 2U)
  {
    I2C2->CR1 |= I2C_CR1_POS;
    I2C2->CR1 &= ~I2C_CR1_ACK;

    timeout = I2C_TIMEOUT;
    while(!(I2C2->SR1 & I2C_SR1_BTF))
    {
      if(--timeout == 0U)
        return I2C_ERR_TIMEOUT;
    }

    I2C2->CR1 |= I2C_CR1_STOP;
    buf[0] = (uint8_t) I2C2->DR;
    buf[1] = (uint8_t) I2C2->DR;

    I2C2->CR1 &= ~I2C_CR1_POS;
  }
  else
  {
    uint8_t i;
    for(i = 0U; i < len - 3U; i++)
    {
      timeout = I2C_TIMEOUT;
      while(!(I2C2->SR1 & I2C_SR1_RXNE))
      {
        if(--timeout == 0U)
          return I2C_ERR_TIMEOUT;
      }
      buf[i] = (uint8_t) I2C2->DR;
    }

    /* Wait for BTF: byte N-2 in DR, byte N-1 in shift register */
    timeout = I2C_TIMEOUT;
    while(!(I2C2->SR1 & I2C_SR1_BTF))
    {
      if(--timeout == 0U)
        return I2C_ERR_TIMEOUT;
    }

    /* NACK the last byte, read byte N-2 */
    I2C2->CR1 &= ~I2C_CR1_ACK;
    buf[len - 3U] = (uint8_t) I2C2->DR;

    /* Wait for RXNE: byte N-1 ready */
    timeout = I2C_TIMEOUT;
    while(!(I2C2->SR1 & I2C_SR1_RXNE))
    {
      if(--timeout == 0U)
        return I2C_ERR_TIMEOUT;
    }
    buf[len - 2U] = (uint8_t) I2C2->DR;

    /* STOP before reading last byte */
    I2C2->CR1 |= I2C_CR1_STOP;

    /* Wait for last byte to be received */
    timeout = I2C_TIMEOUT;
    while(!(I2C2->SR1 & I2C_SR1_RXNE))
    {
      if(--timeout == 0U)
        return I2C_ERR_TIMEOUT;
    }
    buf[len - 1U] = (uint8_t) I2C2->DR;
  }

  return I2C_OK;
}

I2C_Status I2C2_RepeatedStart(void)
{
  uint32_t timeout = I2C_TIMEOUT;

  // Do NOT wait for BUSY to clear
  I2C2->CR1 |= I2C_CR1_START;

  // Wait for Start Bit (SB) flag
  while(!(I2C2->SR1 & I2C_SR1_SB))
  {
    if(--timeout == 0U)
    {
      return I2C_ERR_START;
    }
  }
  return I2C_OK;
}
