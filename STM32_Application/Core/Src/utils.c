/*
 * utils.c
 *
 *  Created on: Feb 20, 2026
 *      Author: Rubin Khadka
 */

#include "utils.h"

void itoa_32(uint32_t value, char *buffer)
{
  char *ptr = buffer;
  char temp[12];  // Enough for 32-bit number
  uint8_t i = 0;

  // Special case for zero
  if(value == 0)
  {
    buffer[0] = '0';
    buffer[1] = '\0';
    return;
  }

  // Extract digits in reverse order
  while(value > 0)
  {
    temp[i++] = (value % 10) + '0';
    value /= 10;
  }

  // Reverse digits into buffer
  while(i-- > 0)
  {
    *ptr++ = temp[i];
  }
  *ptr = '\0';
}

// Convert 16 bit integer to string
void itoa_16(int16_t value, char *buffer)
{
  char *ptr = buffer;

  // Handle negative numbers
  if(value < 0)
  {
    *ptr++ = '-';
    value = -value;
  }

  // Extract digits in reverse order
  char temp[6];
  uint8_t i = 0;
  do
  {
    temp[i++] = (value % 10) + '0';
    value /= 10;
  }
  while(value > 0);

  // Reverse digits into buffer
  while(i-- > 0)
  {
    *ptr++ = temp[i];
  }
  *ptr = '\0';
}

// Convert 8 bit integer to string
void itoa_8(uint8_t value, char *buffer)
{
  char *ptr = buffer;
  char temp[3];
  uint8_t i = 0;

  // Special case for zero
  if(value == 0)
  {
    buffer[0] = '0';
    buffer[1] = '\0';
    return;
  }

  // Extract digits
  do
  {
    temp[i++] = (value % 10) + '0';
    value /= 10;
  }
  while(value > 0);

  // Reverse digits
  while(i-- > 0)
  {
    *ptr++ = temp[i];
  }
  *ptr = '\0';
}

// Format integer value for UART output
void format_value(uint8_t integer, uint8_t decimal, char *buffer, char unit)
{
  char *ptr = buffer;

  // Integer part
  if(integer >= 10)
  {
    *ptr++ = '0' + (integer / 10);
  }
  *ptr++ = '0' + (integer % 10);

  // Decimal point and tenths
  *ptr++ = '.';
  *ptr++ = '0' + decimal;

  // Unit
  *ptr++ = unit;
  *ptr = '\0';
}

void format_reading_temp(uint8_t temp_int, uint8_t temp_dec, uint8_t hum_int, uint8_t hum_dec, char *buffer)
{
  char *ptr = buffer;
  char temp[8];
  uint8_t i;

  // Add "Temp: "
  *ptr++ = 'T';
  *ptr++ = 'e';
  *ptr++ = 'm';
  *ptr++ = 'p';
  *ptr++ = ':';
  *ptr++ = ' ';

  // Format temperature
  format_value(temp_int, temp_dec, temp, 'C');
  i = 0;
  while(temp[i])
  {
    *ptr++ = temp[i++];
  }

  // Add ", Hum: "
  *ptr++ = ',';
  *ptr++ = ' ';
  *ptr++ = 'H';
  *ptr++ = 'u';
  *ptr++ = 'm';
  *ptr++ = ':';
  *ptr++ = ' ';

  // Format humidity
  format_value(hum_int, hum_dec, temp, '%');
  i = 0;
  while(temp[i])
  {
    *ptr++ = temp[i++];
  }

  // Add newline
  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';
}

/* --------------------- Functions for Scaled Values--------------------------- */

// Convert float to string with specified decimal places
void ftoa(float value, char *buffer, uint8_t decimal_places)
{
  char *ptr = buffer;

  // Handle negative numbers
  if(value < 0)
  {
    *ptr++ = '-';
    value = -value;
  }

  // Extract integer part
  int32_t int_part = (int32_t) value;

  // Extract fractional part
  float fractional = value - int_part;

  // Handle integer part
  char temp[16];
  uint8_t i = 0;

  // Special case for zero
  if(int_part == 0)
  {
    temp[i++] = '0';
  }
  else
  {
    // Extract integer digits in reverse
    while(int_part > 0)
    {
      temp[i++] = (int_part % 10) + '0';
      int_part /= 10;
    }
  }

  // Reverse integer digits into buffer
  while(i-- > 0)
  {
    *ptr++ = temp[i];
  }

  // Add decimal point
  *ptr++ = '.';

  // Handle fractional part
  for(uint8_t j = 0; j < decimal_places; j++)
  {
    fractional *= 10;
    uint8_t digit = (uint8_t) fractional;
    *ptr++ = digit + '0';
    fractional -= digit;
  }

  *ptr = '\0';
}

// Format float with unit
void format_float(float value, char *buffer, uint8_t decimal_places, char unit)
{
  char *ptr = buffer;
  char num[16];

  ftoa(value, num, decimal_places);

  // Copy the number
  for(char *s = num; *s; s++)
  {
    *ptr++ = *s;
  }

  // Add unit
  *ptr++ = unit;
  *ptr = '\0';
}

// Format scaled accelerometer data
void format_accel_scaled(char *buffer, float ax, float ay, float az, uint8_t decimal_places)
{
  char *ptr = buffer;
  char num[16];

  // "AX:1.23g AY:4.56g AZ:7.89g"

  // AX
  *ptr++ = 'A';
  *ptr++ = 'X';
  *ptr++ = ':';
  ftoa(ax, num, decimal_places);
  for(char *s = num; *s; s++)
    *ptr++ = *s;
  *ptr++ = 'g';
  *ptr++ = ' ';

  // AY
  *ptr++ = 'A';
  *ptr++ = 'Y';
  *ptr++ = ':';
  ftoa(ay, num, decimal_places);
  for(char *s = num; *s; s++)
    *ptr++ = *s;
  *ptr++ = 'g';
  *ptr++ = ' ';

  // AZ
  *ptr++ = 'A';
  *ptr++ = 'Z';
  *ptr++ = ':';
  ftoa(az, num, decimal_places);
  for(char *s = num; *s; s++)
    *ptr++ = *s;
  *ptr++ = 'g';

  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';
}

// Format scaled gyroscope data
void format_gyro_scaled(char *buffer, float gx, float gy, float gz, uint8_t decimal_places)
{
  char *ptr = buffer;
  char num[16];

  // "GX:1.23dps GY:4.56dps GZ:7.89dps"

  // GX
  *ptr++ = 'G';
  *ptr++ = 'X';
  *ptr++ = ':';
  ftoa(gx, num, decimal_places);
  for(char *s = num; *s; s++)
    *ptr++ = *s;
  *ptr++ = 'd';
  *ptr++ = 'p';
  *ptr++ = 's';
  *ptr++ = ' ';

  // GY
  *ptr++ = 'G';
  *ptr++ = 'Y';
  *ptr++ = ':';
  ftoa(gy, num, decimal_places);
  for(char *s = num; *s; s++)
    *ptr++ = *s;
  *ptr++ = 'd';
  *ptr++ = 'p';
  *ptr++ = 's';
  *ptr++ = ' ';

  // GZ
  *ptr++ = 'G';
  *ptr++ = 'Z';
  *ptr++ = ':';
  ftoa(gz, num, decimal_places);
  for(char *s = num; *s; s++)
    *ptr++ = *s;
  *ptr++ = 'd';
  *ptr++ = 'p';
  *ptr++ = 's';

  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';
}

// Convert a 2-digit number to string
void IntToTwoDigits(uint8_t num, char *str)
{
  str[0] = '0' + (num / 10);  // Tens digit
  str[1] = '0' + (num % 10);  // Ones digit
  str[2] = '\0';               // Null terminator
}

// Convert temperature to string with 1 decimal place
void TemperatureToString(float temp, char *str)
{
  int8_t int_part = (int8_t) temp;
  uint8_t frac_part = (uint8_t) ((temp - int_part) * 10);

  // Handle negative temperatures
  if(int_part < 0)
  {
    str[0] = '-';
    int_part = -int_part;
    IntToTwoDigits(int_part, &str[1]);
    str[3] = '.';
    str[4] = '0' + frac_part;
    str[5] = 'C';
    str[6] = '\0';
  }
  else
  {
    IntToTwoDigits(int_part, str);
    str[2] = '.';
    str[3] = '0' + frac_part;
    str[4] = 'C';
    str[5] = '\0';
  }
}

// Format time string as HH:MM:SS
void FormatTimeString(uint8_t hour, uint8_t minutes, uint8_t seconds, char *buffer)
{
  // Hour
  buffer[0] = '0' + (hour / 10);
  buffer[1] = '0' + (hour % 10);
  buffer[2] = ':';

  // Minutes
  buffer[3] = '0' + (minutes / 10);
  buffer[4] = '0' + (minutes % 10);
  buffer[5] = ':';

  // Seconds
  buffer[6] = '0' + (seconds / 10);
  buffer[7] = '0' + (seconds % 10);
  buffer[8] = '\0';
}

// Format date string as DD/MM/YYYY
void FormatDateString(uint8_t day, uint8_t month, uint8_t year, char *buffer)
{
  // Day
  buffer[0] = '0' + (day / 10);
  buffer[1] = '0' + (day % 10);
  buffer[2] = '/';

  // Month
  buffer[3] = '0' + (month / 10);
  buffer[4] = '0' + (month % 10);
  buffer[5] = '/';

  // Year (20YY)
  buffer[6] = '2';
  buffer[7] = '0';
  buffer[8] = '0' + (year / 10);
  buffer[9] = '0' + (year % 10);
  buffer[10] = '\0';
}

// Format timestamp as YYYY-MM-DD HH:MM:SS
void FormatTimestamp(DS3231_Time_t *time, char *buffer, uint8_t buffer_size)
{
  if(buffer_size < 20)
    return;  // Need at least 20 bytes for "YYYY-MM-DD HH:MM:SS"

  uint8_t pos = 0;

  // Year (20YY)
  buffer[pos++] = '2';
  buffer[pos++] = '0';
  buffer[pos++] = '0' + (time->year / 10);
  buffer[pos++] = '0' + (time->year % 10);
  buffer[pos++] = '-';

  // Month
  buffer[pos++] = '0' + (time->month / 10);
  buffer[pos++] = '0' + (time->month % 10);
  buffer[pos++] = '-';

  // Day
  buffer[pos++] = '0' + (time->dayofmonth / 10);
  buffer[pos++] = '0' + (time->dayofmonth % 10);
  buffer[pos++] = ' ';

  // Hour
  buffer[pos++] = '0' + (time->hour / 10);
  buffer[pos++] = '0' + (time->hour % 10);
  buffer[pos++] = ':';

  // Minute
  buffer[pos++] = '0' + (time->minutes / 10);
  buffer[pos++] = '0' + (time->minutes % 10);
  buffer[pos++] = ':';

  // Second
  buffer[pos++] = '0' + (time->seconds / 10);
  buffer[pos++] = '0' + (time->seconds % 10);
  buffer[pos] = '\0';
}
