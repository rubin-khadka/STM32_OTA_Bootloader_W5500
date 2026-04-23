/*
 * ds3231.h
 *
 *  Created on: Mar 5, 2026
 *      Author: Rubin Khadka
 */

#ifndef DS3231_H_
#define DS3231_H_

#include "stdint.h"

// DS3231 I2C Address (7-bit address)
#define DS3231_ADDR     0x68

// DS3231 Register addresses
#define DS3231_REG_SEC        0x00
#define DS3231_REG_MIN        0x01
#define DS3231_REG_HOUR       0x02
#define DS3231_REG_DOW        0x03
#define DS3231_REG_DATE       0x04
#define DS3231_REG_MONTH      0x05
#define DS3231_REG_YEAR       0x06
#define DS3231_REG_ALARM1     0x07
#define DS3231_REG_ALARM2     0x0B
#define DS3231_REG_CONTROL    0x0E
#define DS3231_REG_STATUS     0x0F
#define DS3231_REG_AGEOFF     0x10
#define DS3231_REG_TEMP_MSB   0x11
#define DS3231_REG_TEMP_LSB   0x12

// Return codes
#define DS3231_OK           0
#define DS3231_ERROR        1
#define DS3231_TIMEOUT      2

// Time structure
typedef struct {
    uint8_t seconds;      // 0-59
    uint8_t minutes;      // 0-59
    uint8_t hour;         // 0-23
    uint8_t dayofweek;    // 1-7 (1 = Sunday)
    uint8_t dayofmonth;   // 1-31
    uint8_t month;        // 1-12
    uint8_t year;         // 0-99 (2000-2099)
} DS3231_Time_t;

extern DS3231_Time_t current_time;

// Function Prototypes
uint8_t DS3231_Init(void);
uint8_t DS3231_SetTime(DS3231_Time_t *time);
uint8_t DS3231_GetTime(DS3231_Time_t *time);
float DS3231_GetTemperature(void);
uint8_t DS3231_ForceTempConv(void);
uint8_t DS3231_SetAlarm1(uint8_t hour, uint8_t min, uint8_t sec, uint8_t mode);
uint8_t DS3231_SetAlarm2(uint8_t hour, uint8_t min, uint8_t mode);
uint8_t DS3231_CheckOscillatorStop(void);

// BCD conversion helpers
uint8_t DS3231_DecToBcd(uint8_t val);
uint8_t DS3231_BcdToDec(uint8_t val);

#endif /* DS3231_H_ */
