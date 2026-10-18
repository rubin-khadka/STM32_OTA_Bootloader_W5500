/*
 * mpu6050.c
 *
 *  Created on: Aug 24, 2026
 *      Author: Rubin Khadka
 */

#include "mpu6050.h"
#include "i2c2.h"
#include "usart1.h"
#include "timer2.h"

// Global variables
volatile MPU6050_RawData_t mpu6050_raw;
volatile MPU6050_ScaledData_t mpu6050_scaled;

// Helper: write a single register
static uint8_t MPU6050_WriteReg(uint8_t reg, uint8_t data) {
  I2C_Status status;

  status = I2C2_Start();
  if (status != I2C_OK) {
    I2C2_Stop();
    return 1;   // error
  }

  status = I2C2_SendAddr(MPU6050_ADDR, I2C_WRITE);
  if (status != I2C_OK) {
    I2C2_Stop();
    return 1;
  }

  status = I2C2_WriteByteWaitComplete(reg);
  if (status != I2C_OK) {
    I2C2_Stop();
    return 1;
  }

  status = I2C2_WriteByteWaitComplete(data);
  if (status != I2C_OK) {
    I2C2_Stop();
    return 1;
  }

  I2C2_Stop();
  return 0;   // success
}

// Helper: read a single register
static uint8_t MPU6050_ReadReg(uint8_t reg, uint8_t *data) {
  if (I2C2_ReadBurst(MPU6050_ADDR, reg, data, 1) != I2C_OK) {
    return 1;   // error
  }
  return 0;       // success
}

// Initialize MPU6050
uint8_t MPU6050_Init(void) {
  uint8_t who_am_i;

  if (MPU6050_ReadReg(MPU6050_WHO_AM_I, &who_am_i) != 0) {
    USART1_SendString("Failed to read WHO_AM_I\r\n");
    return 1;
  }

  if (who_am_i != 0x68 && who_am_i != 0x69) {
    USART1_SendString("Wrong device ID!\r\n");
    return 1;
  }

  // Wake up MPU6050 (clear sleep bit)
  if (MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x00) != 0) {
    USART1_SendString("Failed to wake device\r\n");
    return 1;
  }

  TIMER2_Delay_ms(10);
  return 0;
}

// Read multiple bytes (burst) using I2C2_ReadBurst
static uint8_t MPU6050_ReadBurst(uint8_t start_reg, uint8_t *data, uint8_t len) {
  if (I2C2_ReadBurst(MPU6050_ADDR, start_reg, data, len) != I2C_OK) {
    return 1;
  }
  return 0;
}

// Read all sensor data
uint8_t MPU6050_ReadAll(void) {
  uint8_t buffer[14];

  if (MPU6050_ReadBurst(MPU6050_ACCEL_XOUT_H, buffer, 14) != 0) {
    return 1;
  }

  mpu6050_raw.accel_x = (int16_t) ((buffer[0] << 8) | buffer[1]);
  mpu6050_raw.accel_y = (int16_t) ((buffer[2] << 8) | buffer[3]);
  mpu6050_raw.accel_z = (int16_t) ((buffer[4] << 8) | buffer[5]);
  mpu6050_raw.temp = (int16_t) ((buffer[6] << 8) | buffer[7]);
  mpu6050_raw.gyro_x = (int16_t) ((buffer[8] << 8) | buffer[9]);
  mpu6050_raw.gyro_y = (int16_t) ((buffer[10] << 8) | buffer[11]);
  mpu6050_raw.gyro_z = (int16_t) ((buffer[12] << 8) | buffer[13]);

  return 0;
}

// Read accelerometer only
uint8_t MPU6050_ReadAccel(void) {
  uint8_t buffer[6];

  if (MPU6050_ReadBurst(MPU6050_ACCEL_XOUT_H, buffer, 6) != 0) {
    return 1;
  }

  mpu6050_raw.accel_x = (int16_t) ((buffer[0] << 8) | buffer[1]);
  mpu6050_raw.accel_y = (int16_t) ((buffer[2] << 8) | buffer[3]);
  mpu6050_raw.accel_z = (int16_t) ((buffer[4] << 8) | buffer[5]);

  return 0;
}

// Read gyroscope only
uint8_t MPU6050_ReadGyro(void) {
  uint8_t buffer[6];

  if (MPU6050_ReadBurst(MPU6050_GYRO_XOUT_H, buffer, 6) != 0) {
    return 1;
  }

  mpu6050_raw.gyro_x = (int16_t) ((buffer[0] << 8) | buffer[1]);
  mpu6050_raw.gyro_y = (int16_t) ((buffer[2] << 8) | buffer[3]);
  mpu6050_raw.gyro_z = (int16_t) ((buffer[4] << 8) | buffer[5]);

  return 0;
}

// Read temperature only
uint8_t MPU6050_ReadTemp(void) {
  uint8_t buffer[2];

  if (MPU6050_ReadBurst(MPU6050_TEMP_OUT_H, buffer, 2) != 0) {
    return 1;
  }

  mpu6050_raw.temp = (int16_t) ((buffer[0] << 8) | buffer[1]);

  return 0;
}

// Scale functions
void MPU6050_ScaleAll(void) {
  mpu6050_scaled.accel_x = mpu6050_raw.accel_x / 16384.0f;
  mpu6050_scaled.accel_y = mpu6050_raw.accel_y / 16384.0f;
  mpu6050_scaled.accel_z = mpu6050_raw.accel_z / 16384.0f;

  mpu6050_scaled.gyro_x = mpu6050_raw.gyro_x / 131.0f;
  mpu6050_scaled.gyro_y = mpu6050_raw.gyro_y / 131.0f;
  mpu6050_scaled.gyro_z = mpu6050_raw.gyro_z / 131.0f;

  mpu6050_scaled.temp = (mpu6050_raw.temp / 340.0f) + 36.53f;
}

void MPU6050_ScaleAccel(void) {
  mpu6050_scaled.accel_x = mpu6050_raw.accel_x / 16384.0f;
  mpu6050_scaled.accel_y = mpu6050_raw.accel_y / 16384.0f;
  mpu6050_scaled.accel_z = mpu6050_raw.accel_z / 16384.0f;
}

void MPU6050_ScaleGyro(void) {
  mpu6050_scaled.gyro_x = mpu6050_raw.gyro_x / 131.0f;
  mpu6050_scaled.gyro_y = mpu6050_raw.gyro_y / 131.0f;
  mpu6050_scaled.gyro_z = mpu6050_raw.gyro_z / 131.0f;
}

void MPU6050_ScaleTemp(void) {
  mpu6050_scaled.temp = (mpu6050_raw.temp / 340.0f) + 36.53f;
}

float MPU6050_ConvertTemp(int16_t raw_temp) {
  return (raw_temp / 340.0f) + 36.53f;
}

float MPU6050_ConvertAccel(int16_t raw_accel) {
  return raw_accel / 16384.0f;
}

float MPU6050_ConvertGyro(int16_t raw_gyro) {
  return raw_gyro / 131.0f;
}
