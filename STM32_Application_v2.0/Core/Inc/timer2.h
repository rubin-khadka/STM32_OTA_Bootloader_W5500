/*
 * timer2.h
 *
 *  Created on: Feb 20, 2026
 *      Author: Rubin Khadka
 */

#ifndef TIMER2_H_
#define TIMER2_H_

#include "stdint.h"

void TIMER2_Init(void);
void TIM2_IRQHandler(void);
uint32_t TIMER2_GetMillis(void);
void TIMER2_Delay_ms(uint16_t ms);

#endif /* TIMER2_H_ */
