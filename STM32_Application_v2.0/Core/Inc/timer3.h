/*
 * timer3.h
 *
 *  Created on: Feb 22, 2026
 *      Author: Rubin Khadka
 */

#ifndef TIMER3_H_
#define TIMER3_H_

#include "stm32f103xb.h"
#include "stdint.h"

// Setup TIM3 with fixed 0.1ms precision
void TIMER3_SetupPeriod(uint16_t ms);
uint8_t TIMER3_WaitPeriod(void);

#endif /* TIMER3_H_ */
