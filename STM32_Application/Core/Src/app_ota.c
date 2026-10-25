/*
 * app_ota.c
 *
 *  Created on: Apr 23, 2026
 *      Author: Rubin Khadka
 */

#include <flash_layout.h>
#include "main.h"
#include "app_ota.h"
#include "flash_operations.h"

#define OTA_FLAG_REQUESTED  0x00000000UL

void enable_ota_request(void)
{
  HAL_FLASH_Unlock();
  flash_write_word(APP_HEADER_ADDR, OTA_FLAG_REQUESTED);
  HAL_FLASH_Lock();
}
