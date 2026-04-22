/*
 * w25q64.h
 *
 *  Created on: Apr 22, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_W25Q64_H_
#define INC_W25Q64_H_

void W25Q_Reset(void);
uint32_t W25Q_ReadID(void);

void W25Q_Erase(uint32_t start_addr, uint32_t size);
void W25Q_Write(uint32_t addr, uint8_t *data, uint16_t len);
void W25Q_Read(uint32_t addr, uint8_t *buf, uint16_t len);
void W25Q_FastRead(uint32_t addr, uint8_t *data, uint16_t len);

#endif /* INC_W25Q64_H_ */
