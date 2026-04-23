/*
 * i2c2.c
 *
 *  Created on: Feb 26, 2026
 *      Author: Rubin Khadka
 */

#include "stm32f103xb.h"
#include "i2c2.h"

void I2C2_Init(void)
{
  // Enable Clocks
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

  // Enable I2C2 peripheral
  RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

  // GPIO Configuration for I2C2
  // PB10 SCL
  GPIOB->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
  GPIOB->CRH |= GPIO_CRH_MODE10_1;      // 2 MHz output
  GPIOB->CRH |= GPIO_CRH_CNF10_0 | GPIO_CRH_CNF10_1;  // Alternate function open drain

  // PB11 SDA
  GPIOB->CRH &= ~(GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
  GPIOB->CRH |= GPIO_CRH_MODE11_1;      // 2 MHz output
  GPIOB->CRH |= GPIO_CRH_CNF11_0 | GPIO_CRH_CNF11_1;  // Alternate function open drain

  // I2C Configuration
  // Reset I2C2
  I2C2->CR1 |= I2C_CR1_SWRST;
  for(volatile int i = 0; i < 100; i++);
  I2C2->CR1 &= ~I2C_CR1_SWRST;

  // APB1 Clock Speed
  I2C2->CR2 |= 36;  // 36 MHz

  // Configure for 100kHz I2C
  // CCR = APB1 clock / (2 * desired speed) for standard mode
  I2C2->CCR = 180;        // 36MHz / (2 * 100kHz) = 180

  // TRISE = APB1 clock in MHz + 1
  I2C2->TRISE = 37;

  // Set duty cycle
  I2C2->CCR &= ~I2C_CCR_DUTY;

  // Disable general call, no stretch, dual address disable
  I2C2->CR1 &= ~(I2C_CR1_ENGC | I2C_CR1_NOSTRETCH);

  // Set own address to 0
  I2C2->OAR1 = 0x4000;

  // Enable I2C with ACK
  I2C2->CR1 |= I2C_CR1_PE | I2C_CR1_ACK;
}

void I2C2_Start(void)
{
  uint32_t timeout = 10000;

  // Wait for bus to be free
  while(I2C2->SR2 & I2C_SR2_BUSY)
  {
    if(--timeout == 0)
      break;
  }

  // Generate start condition
  I2C2->CR1 |= I2C_CR1_START;

  // Wait for start condition generated (SB bit)
  timeout = 10000;
  while(!(I2C2->SR1 & I2C_SR1_SB))
  {
    if(--timeout == 0)
      break;
  }
}

void I2C2_Stop(void)
{
  // Generate stop condition
  I2C2->CR1 |= I2C_CR1_STOP;

  // Small delay to let STOP complete
  for(volatile int i = 0; i < 100; i++);
}

uint8_t I2C2_SendAddr(uint8_t addr, uint8_t rw)
{
  uint32_t timeout = 10000;

  // Clear any pending acknowledge failure
  I2C2->SR1 &= ~I2C_SR1_AF;

  // Send address (shifted left 1 bit + R/W bit)
  I2C2->DR = (addr << 1) | rw;

  // Wait for ADDR flag (address sent and ACK received)
  while(!(I2C2->SR1 & I2C_SR1_ADDR))
  {
    // Check for acknowledge failure
    if(I2C2->SR1 & I2C_SR1_AF)
    {
      return I2C_ERROR;  // No ACK from device
    }

    if(--timeout == 0)
    {
      return I2C_TIMEOUT;
    }
  }

  // Clear ADDR flag by reading SR2 register
  (void) I2C2->SR2;

  return I2C_OK;
}

uint8_t I2C2_WriteByte(uint8_t data)
{
  uint32_t timeout = 10000;

  // Wait for TXE flag (data register empty)
  while(!(I2C2->SR1 & I2C_SR1_TXE))
  {
    // Check for acknowledge failure
    if(I2C2->SR1 & I2C_SR1_AF)
    {
      return I2C_ERROR;
    }

    if(--timeout == 0)
    {
      return I2C_TIMEOUT;
    }
  }

  // Send data
  I2C2->DR = data;

  // Wait for byte to be transmitted (TXE set again)
  timeout = 10000;
  while(!(I2C2->SR1 & I2C_SR1_TXE))
  {
    // Check for acknowledge failure
    if(I2C2->SR1 & I2C_SR1_AF)
    {
      return I2C_ERROR;
    }

    if(--timeout == 0)
    {
      return I2C_TIMEOUT;
    }
  }

  return I2C_OK;
}

uint8_t I2C2_ReadByte(uint8_t ack)
{
  uint32_t timeout = 10000;

  // Configure ACK/NACK for this byte
  if(ack)
  {
    I2C2->CR1 |= I2C_CR1_ACK;   // Send ACK after receiving
  }
  else
  {
    I2C2->CR1 &= ~I2C_CR1_ACK;  // Send NACK after receiving
  }

  // Wait for RXNE flag (data received)
  while(!(I2C2->SR1 & I2C_SR1_RXNE))
  {
    if(--timeout == 0)
    {
      return 0;
    }
  }

  // Read received data
  return I2C2->DR;
}

