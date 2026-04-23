/*
 * ds3231.c
 *
 *  Created on: Mar 5, 2026
 *      Author: Rubin Khadka
 */

#include "ds3231.h"
#include "i2c2.h"

DS3231_Time_t current_time;

// Internal helper functions
static uint8_t DS3231_WriteReg(uint8_t reg, uint8_t data);
static uint8_t DS3231_ReadReg(uint8_t reg, uint8_t *data);
static uint8_t DS3231_WriteMulti(uint8_t reg, uint8_t *data, uint8_t len);
static uint8_t DS3231_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len);

// Initialize DS3231
uint8_t DS3231_Init(void)
{
  uint8_t status;
  uint8_t is_first_boot = 0;

  // Read status register to check if oscillator is running
  if(DS3231_ReadReg(DS3231_REG_STATUS, &status) != DS3231_OK)
    return DS3231_ERROR;

  // Check oscillator stop flag (bit 7 in status register)
  if(status & 0x80)
  {
    // Oscillator has stopped - this means power was lost or first boot
    is_first_boot = 1;

    // Clear the flag
    status &= ~0x80;
    DS3231_WriteReg(DS3231_REG_STATUS, status);
  }

  // Only set initial time on first boot OR if time registers are invalid
  if(is_first_boot)
  {
    // Set initial time (only on first power-up or after battery failure)
    current_time.seconds = 00;
    current_time.minutes = 00;
    current_time.hour = 22;
    current_time.dayofweek = 5;
    current_time.dayofmonth = 17;
    current_time.month = 04;
    current_time.year = 26;  // 2026

    DS3231_SetTime(&current_time);
  }
  else
  {
    // Read the current time from DS3231
    DS3231_GetTime(&current_time);
  }

  return DS3231_OK;
}

// Convert decimal to binary coded decimal (BCD)
uint8_t DS3231_DecToBcd(uint8_t val)
{
  return ((val / 10) << 4) | (val % 10);
}

// Convert binary coded decimal (BCD) to decimal
uint8_t DS3231_BcdToDec(uint8_t val)
{
  return ((val >> 4) * 10) + (val & 0x0F);
}

// Set DS3231 time
uint8_t DS3231_SetTime(DS3231_Time_t *time)
{
  uint8_t data[7];

  // Convert decimal to BCD
  data[0] = DS3231_DecToBcd(time->seconds);
  data[1] = DS3231_DecToBcd(time->minutes);
  data[2] = DS3231_DecToBcd(time->hour);
  data[3] = DS3231_DecToBcd(time->dayofweek);
  data[4] = DS3231_DecToBcd(time->dayofmonth);
  data[5] = DS3231_DecToBcd(time->month);
  data[6] = DS3231_DecToBcd(time->year);

  // Write all 7 time registers starting from 0x00
  return DS3231_WriteMulti(DS3231_REG_SEC, data, 7);
}

// Get current time from DS3231
uint8_t DS3231_GetTime(DS3231_Time_t *time)
{
  uint8_t data[7];

  // Read all 7 time registers starting from 0x00
  if(DS3231_ReadMulti(DS3231_REG_SEC, data, 7) != DS3231_OK)
    return DS3231_ERROR;

  // Convert BCD to decimal with proper masking
  time->seconds = DS3231_BcdToDec(data[0] & 0x7F);     // Mask out CH bit
  time->minutes = DS3231_BcdToDec(data[1] & 0x7F);
  time->hour = DS3231_BcdToDec(data[2] & 0x3F);       // Mask out 12/24 bit
  time->dayofweek = DS3231_BcdToDec(data[3] & 0x07);
  time->dayofmonth = DS3231_BcdToDec(data[4] & 0x3F);
  time->month = DS3231_BcdToDec(data[5] & 0x1F);
  time->year = DS3231_BcdToDec(data[6]);

  return DS3231_OK;
}

// Get temperature from DS3231 internal sensor
float DS3231_GetTemperature(void)
{
  uint8_t temp_msb, temp_lsb;
  float temperature;

  // Read temperature registers
  if(DS3231_ReadReg(DS3231_REG_TEMP_MSB, &temp_msb) != DS3231_OK)
    return 0.0f;

  if(DS3231_ReadReg(DS3231_REG_TEMP_LSB, &temp_lsb) != DS3231_OK)
    return 0.0f;

  // Temperature format:
  // MSB = integer part, LSB bits 7-6 = fractional (0.25°C steps)
  temperature = (float) temp_msb;
  temperature += ((float) (temp_lsb >> 6)) * 0.25f;

  return temperature;
}

// Force DS3231 to perform a temperature conversion
uint8_t DS3231_ForceTempConv(void)
{
  uint8_t status, control;

  // Read status register
  if(DS3231_ReadReg(DS3231_REG_STATUS, &status) != DS3231_OK)
    return DS3231_ERROR;

  // Check if conversion is already in progress (BSY bit)
  if(!(status & 0x04))
  {
    // Read control register
    if(DS3231_ReadReg(DS3231_REG_CONTROL, &control) != DS3231_OK)
      return DS3231_ERROR;

    // Set CONV bit (bit 5) to start conversion
    control |= 0x20;
    if(DS3231_WriteReg(DS3231_REG_CONTROL, control) != DS3231_OK)
      return DS3231_ERROR;
  }

  return DS3231_OK;
}

// Check if oscillator has stopped (power failure)
uint8_t DS3231_CheckOscillatorStop(void)
{
  uint8_t status;

  if(DS3231_ReadReg(DS3231_REG_STATUS, &status) != DS3231_OK)
    return 1;  // Assume stopped on error

  return (status & 0x80) ? 1 : 0;
}

// Set Alarm 1 (second, minute, hour, day/date)
uint8_t DS3231_SetAlarm1(uint8_t hour, uint8_t min, uint8_t sec, uint8_t mode)
{
  uint8_t alarm[4];

  // Convert to BCD
  alarm[0] = DS3231_DecToBcd(sec);   // Seconds
  alarm[1] = DS3231_DecToBcd(min);   // Minutes
  alarm[2] = DS3231_DecToBcd(hour);  // Hours
  alarm[3] = mode;                   // Day/date with mode bits

  // Write to alarm1 registers
  return DS3231_WriteMulti(DS3231_REG_ALARM1, alarm, 4);
}

// Set Alarm 2 (minute, hour, day/date)
uint8_t DS3231_SetAlarm2(uint8_t hour, uint8_t min, uint8_t mode)
{
  uint8_t alarm[3];

  // Convert to BCD
  alarm[0] = DS3231_DecToBcd(min);   // Minutes
  alarm[1] = DS3231_DecToBcd(hour);  // Hours
  alarm[2] = mode;                   // Day/date with mode bits

  // Write to alarm2 registers
  return DS3231_WriteMulti(DS3231_REG_ALARM2, alarm, 3);
}

// Write a single register on DS3231

static uint8_t DS3231_WriteReg(uint8_t reg, uint8_t data)
{
  uint8_t result;

  I2C2_Start();

  result = I2C2_SendAddr(DS3231_ADDR, I2C_WRITE);
  if(result != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Send register address
  if(I2C2_WriteByte(reg) != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Send data
  if(I2C2_WriteByte(data) != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  I2C2_Stop();
  return DS3231_OK;
}

// Read a single register from DS3231
static uint8_t DS3231_ReadReg(uint8_t reg, uint8_t *data)
{
  uint8_t result;

  // Start condition
  I2C2_Start();

  // Send device address with write bit to set register pointer
  result = I2C2_SendAddr(DS3231_ADDR, I2C_WRITE);
  if(result != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Send register address to read from
  if(I2C2_WriteByte(reg) != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Restart condition to switch to read mode
  I2C2_Start();

  // Send device address with read bit
  result = I2C2_SendAddr(DS3231_ADDR, I2C_READ);
  if(result != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Read data with NACK (last byte)
  *data = I2C2_ReadByte(0);  // 0 = NACK

  I2C2_Stop();

  return DS3231_OK;
}

// Write multiple registers starting from 'reg'
static uint8_t DS3231_WriteMulti(uint8_t reg, uint8_t *data, uint8_t len)
{
  uint8_t result;
  uint8_t i;

  I2C2_Start();

  result = I2C2_SendAddr(DS3231_ADDR, I2C_WRITE);
  if(result != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Send starting register address
  if(I2C2_WriteByte(reg) != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Send all data bytes
  for(i = 0; i < len; i++)
  {
    if(I2C2_WriteByte(data[i]) != I2C_OK)
    {
      I2C2_Stop();
      return DS3231_ERROR;
    }
  }

  I2C2_Stop();
  return DS3231_OK;
}

// Read multiple registers starting from 'reg'
static uint8_t DS3231_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len)
{
  uint8_t result;
  uint8_t i;

  // Start condition
  I2C2_Start();

  // Send device address with write bit to set register pointer
  result = I2C2_SendAddr(DS3231_ADDR, I2C_WRITE);
  if(result != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Send starting register address
  if(I2C2_WriteByte(reg) != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Restart condition to switch to read mode
  I2C2_Start();

  // Send device address with read bit
  result = I2C2_SendAddr(DS3231_ADDR, I2C_READ);
  if(result != I2C_OK)
  {
    I2C2_Stop();
    return DS3231_ERROR;
  }

  // Read all bytes (with ACK for all but last)
  for(i = 0; i < len; i++)
  {
    if(i == (len - 1))
    {
      // Last byte, send NACK
      data[i] = I2C2_ReadByte(0);
    }
    else
    {
      // All other bytes, send ACK
      data[i] = I2C2_ReadByte(1);
    }
  }

  I2C2_Stop();

  return DS3231_OK;
}
