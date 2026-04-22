/*
 * flash_operations.h
 *
 *  Created on: Apr 22, 2026
 *      Author: rubin
 */

#ifndef INC_FLASH_OPERATIONS_H_
#define INC_FLASH_OPERATIONS_H_

uint32_t flash_read_ota_flag(void);
uint32_t flash_erase_app (void);
void flash_write_word(uint32_t addr, uint32_t data);
uint32_t flash_erase_header (void);

#endif /* INC_FLASH_OPERATIONS_H_ */
