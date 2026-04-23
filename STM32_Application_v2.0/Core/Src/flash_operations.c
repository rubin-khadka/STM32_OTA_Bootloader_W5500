/*
 * flash_operations.c
 *
 *  Created on: Apr 23, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "ota_tcp_server.h"
#include "flash_operations.h"
#include "flash_layout.h"
#include "string.h"

uint32_t flash_read_ota_flag(void)
{
  __IO uint32_t *flash_ptr = (__IO uint32_t*) APP_HEADER_ADDR;
  return flash_ptr[0];
}

uint32_t flash_erase_app(void)
{
  FLASH_EraseInitTypeDef erase;
  uint32_t error;

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.PageAddress = APP_START_ADDR;
  erase.NbPages = APP_MAX_SIZE / 1024;

  HAL_FLASHEx_Erase(&erase, &error);
  return error;
}

//uint32_t flash_erase_ota (void)
//{
//    FLASH_EraseInitTypeDef erase;
//    uint32_t error;
//
//    erase.TypeErase     = FLASH_TYPEERASE_PAGES;
//    erase.PageAddress   = OTA_START_ADDR;
//    erase.NbPages       = OTA_MAX_SIZE/1024;
//
//    HAL_FLASHEx_Erase(&erase, &error);
//    return error;
//}

uint32_t flash_erase_header(void)
{
  FLASH_EraseInitTypeDef erase;
  uint32_t error;

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.PageAddress = APP_HEADER_ADDR;
  erase.NbPages = 1;

  HAL_FLASHEx_Erase(&erase, &error);
  return error;
}

void flash_write_word(uint32_t addr, uint32_t data)
{
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data);
}

void flash_write_bytes(uint32_t addr, uint8_t *buf, uint32_t len)
{
  uint32_t aligned_addr = addr & ~3;
  uint32_t padding = addr - aligned_addr;
  uint32_t temp_word;
  uint32_t i = 0;

  // Handle unaligned start
  if(padding)
  {
    temp_word = 0xFFFFFFFF;
    memcpy(((uint8_t*) &temp_word) + padding, buf, 4 - padding);
    flash_write_word(aligned_addr, temp_word);
    aligned_addr += 4;
    i += 4 - padding;
  }

  // Write full words
  for(; i + 4 <= len; i += 4)
  {
    memcpy(&temp_word, buf + i, 4);
    flash_write_word(aligned_addr, temp_word);
    aligned_addr += 4;
  }

  // Last partial word
  if(i < len)
  {
    temp_word = 0xFFFFFFFF;
    memcpy(&temp_word, buf + i, len - i);
    flash_write_word(aligned_addr, temp_word);
  }
}

