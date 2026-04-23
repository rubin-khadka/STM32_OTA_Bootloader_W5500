/*
 * timer2.c
 *
 *  Created on: Feb 20, 2026
 *      Author: Rubin Khadka
 */

#include "stm32f103xb.h"
#include "timer2.h"

// Millisecond counter
static volatile uint32_t system_millis = 0;

void TIMER2_Init(void)
{
  // Enable TIM2 clock
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  // Small delay for clock to stabilize
  for(volatile int i = 0; i < 10; i++);

  // Configure for 1ms resolution at 72MHz
  TIM2->PSC = 7200 - 1;  // Prescaler = 7199

  // Auto-reload for 1ms (10 ticks at 10kHz = 1ms)
  TIM2->ARR = 10 - 1;

  // Clear counter
  TIM2->CNT = 0;

  // Clear update flag
  TIM2->SR &= ~TIM_SR_UIF;

  // Enable update interrupt
  TIM2->DIER |= TIM_DIER_UIE;

  // Enable TIM2 interrupt in NVIC
  NVIC_EnableIRQ(TIM2_IRQn);
  NVIC_SetPriority(TIM2_IRQn, 1);

  // Start timer
  TIM2->CR1 |= TIM_CR1_CEN;
}

// TIM2 Interrupt Handler
void TIM2_IRQHandler(void)
{
  // Check if update interrupt flag is set
  if(TIM2->SR & TIM_SR_UIF)
  {
    // Clear the interrupt flag
    TIM2->SR &= ~TIM_SR_UIF;

    // Increment millisecond counter
    system_millis++;
  }
}

// Get current system time in milliseconds
uint32_t TIMER2_GetMillis(void)
{
  uint32_t ms;

  // Disable interrupts to read consistent value
  __disable_irq();
  ms = system_millis;
  __enable_irq();

  return ms;
}

// Blocking Delay
void TIMER2_Delay_ms(uint16_t ms)
{
  uint32_t start = TIMER2_GetMillis();

  // Wait until required milliseconds have passed
  while((TIMER2_GetMillis() - start) < ms);
}

