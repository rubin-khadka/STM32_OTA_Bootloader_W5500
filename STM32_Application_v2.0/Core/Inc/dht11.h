/*
 * dht11.h
 *
 *  Created on: Feb 19, 2026
 *      Author: Rubin Khadka
 */

#ifndef DHT11_H
#define DHT11_H

void DHT11_Init(void);
int DHT11_Start(void);
int DHT11_Check_Response(void);
uint8_t DHT11_Read(void);

#endif
