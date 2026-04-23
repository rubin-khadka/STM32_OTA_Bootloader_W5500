/*
 * utils.h
 *
 *  Created on: Feb 20, 2026
 *      Author: Rubin Khadka
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "stdint.h"
#include "ds3231.h"

// Function Prototypes
// Integer to string utility functions to display raw values
void itoa_32(uint32_t value, char *buffer);
void itoa_16(int16_t value, char *buffer);
void itoa_8(uint8_t value, char *buffer);
void format_value(uint8_t integer, uint8_t decimal, char *buffer, char unit);

// Float to string utility functions to display scaled values
void format_reading_temp(uint8_t temp_int, uint8_t temp_dec, uint8_t hum_int, uint8_t hum_dec, char *buffer);
void ftoa(float value, char *buffer, uint8_t decimal_places);
void format_float(float value, char *buffer, uint8_t decimal_places, char unit);
void format_accel_scaled(char *buffer, float ax, float ay, float az, uint8_t decimal_places);
void format_gyro_scaled(char *buffer, float gx, float gy, float gz, uint8_t decimal_places);

void IntToTwoDigits(uint8_t num, char* str);
void TemperatureToString(float temp, char* str);
void FormatTimeString(uint8_t hour, uint8_t minutes, uint8_t seconds, char* buffer);
void FormatDateString(uint8_t day, uint8_t month, uint8_t year, char* buffer);
void FormatTimestamp(DS3231_Time_t *time, char *buffer, uint8_t buffer_size);

#endif /* UTILS_H_ */
