/*
 * bl_jump.c
 *
 *  Created on: Apr 16, 2026
 *      Author: Rubin Khadka
 */

#include "stm32f103xb.h"
#include "flash_layout.h"
#include "bl_jump.h"
#include "app_header.h"
#include "crc32.h"

#define APP_MAGIC     0xABCDEFAB

typedef void (*pFunction)(void);

void Jump_To_Application(void)
{
  uint32_t appStack;
  uint32_t appResetHandler;
  pFunction appEntry;

  /* Read application stack pointer */
  appStack = *(volatile uint32_t*) APP_START_ADDR;

  /* Read reset handler address */
  appResetHandler = *(volatile uint32_t*) (APP_START_ADDR + 4);
  appEntry = (pFunction) appResetHandler;

  /* Disable interrupts */
  __disable_irq();
  __DSB();  // Data Synchronization Barrier
  __ISB();  // Instruction Synchronization Barrier

  /* Stop SysTick */
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  /* Set main stack pointer */
  __set_MSP(appStack);

  /* Jump to application reset handler */
  appEntry();
}

int Bootloader_Is_App_Valid(void)
{
  uint32_t HDR_ADDR = APP_HEADER_ADDR;
  const app_header_t *app_hdr = (const app_header_t*) HDR_ADDR;

  /* 1. Magic */
  if(app_hdr->magic != APP_MAGIC)
    return MAGIC_ERROR;

  /* 2. Reset handler sanity */
  uint32_t reset_handler = *(uint32_t*) (APP_START_ADDR + 4);
  if((reset_handler & 0xFF000000) != 0x08000000)
    return RESET_ERROR;

  /* 3. Size sanity */
  if(app_hdr->size == 0 || app_hdr->size > APP_MAX_SIZE)
    return SIZE_ERROR;

  /* 4. CRC check */
  uint32_t calc_crc = crc32((const uint8_t*) APP_START_ADDR, app_hdr->size);

  if(calc_crc != app_hdr->crc)
    return CRC_ERROR;

  return BOOT_SUCCESS;
}
