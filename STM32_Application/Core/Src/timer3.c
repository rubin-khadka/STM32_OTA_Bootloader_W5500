/*
 * timer3.c
 *
 *  Created on: Feb 22, 2026
 *      Author: Rubin Khadka
 */

#include "timer3.h"

// Fixed prescaler for 0.1ms precision at 72MHz
// 72MHz / 7200 = 10kHz = 0.1ms per tick
#define TIM3_PRESCALER   7200  // 7200-1 = 7199 in register

static uint16_t period_ticks = 0;

void TIMER3_SetupPeriod(uint16_t ms)
{
  // Enable TIM3 clock
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

  // Stop timer if running
  TIM3->CR1 &= ~TIM_CR1_CEN;

  // Calculate ticks needed for desired ms
  // Each tick = 0.1ms, so 10 ticks = 1ms
  period_ticks = ms * 10;  // Convert ms to 0.1ms ticks

  // Set fixed prescaler for 0.1ms precision
  TIM3->PSC = 7200 - 1;  // 7199

  // Set period
  TIM3->ARR = period_ticks - 1;

  // Reset counter
  TIM3->CNT = 0;

  // Clear any pending flags
  TIM3->SR &= ~TIM_SR_UIF;

  // Disable interrupts (using polling)
  TIM3->DIER &= ~TIM_DIER_UIE;

  // Start timer
  TIM3->CR1 |= TIM_CR1_CEN;
}

uint8_t TIMER3_WaitPeriod(void)
{
  // Check if already expired
  if(TIM3->SR & TIM_SR_UIF)
  {
    TIM3->SR &= ~TIM_SR_UIF;  // Clear flag
    return 1;  // Already expired
  }

  // Wait for period
  while(!(TIM3->SR & TIM_SR_UIF));

  // Clear flag
  TIM3->SR &= ~TIM_SR_UIF;

  return 0;  // Expired after waiting
}
