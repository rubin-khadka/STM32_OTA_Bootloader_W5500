/*
 * mpu6050.h
 *
 *  Created on: Feb 21, 2026
 *      Author: Rubin Khadka
 */

#ifndef MPU6050_H_
#define MPU6050_H_

#include "stm32f103xb.h"
#include "stdint.h"

// MPU6050 I2C address (7-bit)
#define MPU6050_ADDR    0x69

// Register addresses
#define MPU6050_WHO_AM_I        0x75
#define MPU6050_PWR_MGMT_1      0x6B
#define MPU6050_ACCEL_XOUT_H    0x3B
#define MPU6050_ACCEL_XOUT_L    0x3C
#define MPU6050_ACCEL_YOUT_H    0x3D
#define MPU6050_ACCEL_YOUT_L    0x3E
#define MPU6050_ACCEL_ZOUT_H    0x3F
#define MPU6050_ACCEL_ZOUT_L    0x40
#define MPU6050_TEMP_OUT_H      0x41
#define MPU6050_TEMP_OUT_L      0x42
#define MPU6050_GYRO_XOUT_H     0x43
#define MPU6050_GYRO_XOUT_L     0x44
#define MPU6050_GYRO_YOUT_H     0x45
#define MPU6050_GYRO_YOUT_L     0x46
#define MPU6050_GYRO_ZOUT_H     0x47
#define MPU6050_GYRO_ZOUT_L     0x48
#define MPU6050_CONFIG          0x1A
#define MPU6050_GYRO_CONFIG     0x1B
#define MPU6050_ACCEL_CONFIG    0x1C

// Raw data structure
typedef struct
{
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t temp;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
} MPU6050_RawData_t;

// Scaled data structure
typedef struct
{
  float accel_x;      // in g
  float accel_y;      // in g
  float accel_z;      // in g
  float temp;         // in °C
  float gyro_x;       // in °/s
  float gyro_y;       // in °/s
  float gyro_z;       // in °/s
} MPU6050_ScaledData_t;

extern volatile MPU6050_RawData_t mpu6050_raw;
extern volatile MPU6050_ScaledData_t mpu6050_scaled;

// Function prototypes
uint8_t MPU6050_Init(void);

// Read functions (read and store raw data)
uint8_t MPU6050_ReadAll(void);
uint8_t MPU6050_ReadAccel(void);
uint8_t MPU6050_ReadGyro(void);
uint8_t MPU6050_ReadTemp(void);

// Scale functions (convert raw to scaled)
void MPU6050_ScaleAll(void);
void MPU6050_ScaleAccel(void);
void MPU6050_ScaleGyro(void);
void MPU6050_ScaleTemp(void);

// Individual conversion functions
float MPU6050_ConvertTemp(int16_t raw_temp);
float MPU6050_ConvertAccel(int16_t raw_accel);
float MPU6050_ConvertGyro(int16_t raw_gyro);

#endif /* MPU6050_H_ */
