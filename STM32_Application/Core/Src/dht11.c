/*
 * dht11.c
 *
 *  Created on: Feb 19, 2026
 *      Author: Rubin Khadka
 */

#include "stm32f103xb.h"
#include "dwt.h"

// Pin definitions
#define DHT11_GPIO      GPIOB
#define DHT11_PIN       0

// Pin operations
#define DHT11_HIGH()    (DHT11_GPIO->BSRR = GPIO_BSRR_BS0)
#define DHT11_LOW()     (DHT11_GPIO->BRR = GPIO_BRR_BR0)
#define DHT11_READ()    ((DHT11_GPIO->IDR >> DHT11_PIN) & 1)

void DHT11_Init(void)
{
  // Enable GPIOB clock
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

  // Set as output, high initially
  DHT11_GPIO->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
  DHT11_GPIO->CRL |= GPIO_CRL_MODE0_0;  // Output 10MHz
  DHT11_HIGH();
}

void DHT11_Start(void)
{
  // Set as output
  DHT11_GPIO->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
  DHT11_GPIO->CRL |= GPIO_CRL_MODE0_0;  // Output

  // Pull LOW for 18ms
  DHT11_LOW();
  DWT_Delay_ms(18);

  // Pull HIGH for 20µs
  DHT11_HIGH();
  DWT_Delay_us(20);

  // Set as input (let sensor respond)
  DHT11_GPIO->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
  DHT11_GPIO->CRL |= GPIO_CRL_CNF0_0;  // Input floating
}

int DHT11_Check_Response(void)
{
  DWT_Delay_us(40);

  // Check if sensor pulled LOW
  if(!DHT11_READ())
  {
    DWT_Delay_us(80);  // Wait 80µs (LOW pulse)

    if(DHT11_READ())  // Should be HIGH now
    {
      // Wait for HIGH to end
      uint32_t timeout = 500;
      while(DHT11_READ())
      {
        if(--timeout == 0)
          return 0;
      }
      return 1;  // Response OK
    }
  }
  return 0;  // No response
}

uint8_t DHT11_Read(void)
{
  uint8_t data = 0;
  uint32_t timeout;

  for(int bit = 7; bit >= 0; bit--)
  {
    // Wait for pin to go HIGH (start of bit)
    timeout = 500;
    while(!DHT11_READ())
    {
      if(--timeout == 0)
        return 0;
    }

    // Wait 40µs into the HIGH pulse
    DWT_Delay_us(40);

    // Check if pin is still HIGH
    if(DHT11_READ())
    {
      // Still HIGH after 40µs = this is a 1 (70µs pulse)
      data |= (1 << bit);

      // Wait for the rest of the HIGH pulse to end
      timeout = 500;
      while(DHT11_READ())
      {
        if(--timeout == 0)
          return 0;
      }
    }
    else
    {
      // Pin went LOW already = this is a 0 (26µs pulse)
      // No need to set bit, just wait for next bit
      // The pin is already LOW, so next bit will start soon
    }
  }

  return data;
}
