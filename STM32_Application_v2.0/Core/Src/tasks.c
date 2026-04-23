/*
 * tasks.c
 *
 *  Created on: Mar 8, 2026
 *      Author: Rubin Khadka
 */

#include "stm32f103xb.h"
#include "dht11.h"
#include "dwt.h"
#include "lcd.h"
#include "ds3231.h"
#include "button.h"
#include "timer2.h"
#include "utils.h"

#define MAX_RETRIES 5

// Global variables to store DHT11 data
static float dht11_humidity = 0.0f;
static float dht11_temperature = 0.0f;

// Global variables to store DHT11 data
volatile uint8_t dht11_humidity1 = 0;
volatile uint8_t dht11_humidity2 = 0;
volatile uint8_t dht11_temperature1 = 0;
volatile uint8_t dht11_temperature2 = 0;

void Task_DHT11_Read(void)
{
  uint8_t hum1, hum2, temp1, temp2, checksum;

  // Disable interrupts for critical section
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  // Try up to MAX_RETRIES times
  for(int retry = 0; retry < MAX_RETRIES; retry++)
  {
    DHT11_Start();

    if(DHT11_Check_Response())
    {
      hum1 = DHT11_Read();
      hum2 = DHT11_Read();
      temp1 = DHT11_Read();
      temp2 = DHT11_Read();
      checksum = DHT11_Read();

      uint8_t calc = hum1 + hum2 + temp1 + temp2;

      if(calc == checksum)
      {
        dht11_humidity1 = hum1;
        dht11_humidity2 = hum2;
        dht11_temperature1 = temp1;
        dht11_temperature2 = temp2;

        // Humidity: combine integer and decimal parts
        dht11_humidity = (float) hum1 + (float) hum2 / 10.0f;

        // Temperature: check sign bit (0x80) for negative values
        if(temp1 & 0x80)
          dht11_temperature = -((float) (temp1 & 0x7F) + (float) temp2 / 10.0f);
        else
          dht11_temperature = (float) temp1 + (float) temp2 / 10.0f;

        break;
      }
    }
  }

  // Re-enable interrupts
  __set_PRIMASK(primask);
}

// Task to update LCD display
void Task_LCD_Update(void)
{
  DisplayMode_t mode = Button_GetMode();

  switch(mode)
  {
    case DISPLAY_MODE_TEMP_HUM:
      LCD_DisplayReading_Temp(dht11_temperature1, dht11_temperature2, dht11_humidity1, dht11_humidity2);
      break;

    case DISPLAY_MODE_DATE_TIME:
      // Get current time
      if(DS3231_GetTime(&current_time) == DS3231_OK)
      {
        static char buffer[17];
        // Format and display time on LCD
        FormatTimeString(current_time.hour, current_time.minutes, current_time.seconds, buffer);
        LCD_SetCursor(0, 0);
        LCD_SendString(buffer);

        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");

        // Format and display date
        FormatDateString(current_time.dayofmonth, current_time.month, current_time.year, buffer);
        LCD_SetCursor(1, 0);
        LCD_SendString(buffer);
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
      }
      break;

    default:  // Handles DISPLAY_MODE_COUNT and any invalid values
      break;
  }
}

