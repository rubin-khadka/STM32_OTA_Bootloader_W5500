/*
 * flash_operations.c
 *
 *  Created on: Apr 22, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "flash_operations.h"
#include "flash_layout.h"

uint32_t flash_read_ota_flag(void) {
	__IO uint32_t *flash_ptr = (__IO uint32_t*) APP_HEADER_ADDR;
	return flash_ptr[0];
}

uint32_t flash_erase_app(void) {
	FLASH_EraseInitTypeDef erase;
	uint32_t error;

	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	erase.PageAddress = APP_START_ADDR;
	erase.NbPages = (APP_MAX_SIZE) / FLASH_PAGE_SIZE;

	HAL_FLASHEx_Erase(&erase, &error);
	return error;
}

uint32_t flash_erase_header(void) {
	FLASH_EraseInitTypeDef erase;
	uint32_t error;

	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	erase.PageAddress = APP_HEADER_ADDR;
	erase.NbPages = 1;

	HAL_FLASHEx_Erase(&erase, &error);
	return error;
}

void flash_write_word(uint32_t addr, uint32_t data) {
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data);
}

