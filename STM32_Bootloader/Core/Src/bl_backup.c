/*
 * bl_backup.c
 *
 *  Created on: Oct 24, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "bl_backup.h"
#include "bl_ota.h"
#include "flash_layout.h"
#include "flash_operations.h"
#include "app_header.h"
#include "w25q64.h"
#include "usart1.h"
#include "crc32.h"
#include <string.h>

#define W25Q_SECTOR_SIZE   4096
#define W25Q_PAGE_SIZE     256
#define BACKUP_HDR_PAD     0x100

// Helpers
static uint32_t sectors_for(uint32_t bytes)
{
  return (bytes + W25Q_SECTOR_SIZE - 1) / W25Q_SECTOR_SIZE;
}

/* CRC32 over a range in W25Q */
static uint32_t crc_of_w25q(uint32_t addr, uint32_t size)
{
  uint32_t crc;
  uint8_t buf[W25Q_PAGE_SIZE];

  bl_crc_init(&crc);
  for(uint32_t off = 0; off < size; off += W25Q_PAGE_SIZE)
  {
    uint32_t n = (size - off >= W25Q_PAGE_SIZE) ? W25Q_PAGE_SIZE : (size - off);
    W25Q_FastRead(addr + off, buf, n);
    bl_crc_update(&crc, buf, n);
  }
  return bl_crc_finalize(crc);
}

/* Copy current app header from internal flash */
static void read_current_header(ota_image_hdr_t *out)
{
  const app_header_t *h = (const app_header_t*) APP_HEADER_ADDR;
  out->magic = h->magic;
  out->image_size = h->size;
  out->crc = h->crc;
  out->version = h->version;
}

// Public API
bool bl_backup_available(void)
{
  ota_image_hdr_t hdr;
  W25Q_FastRead(BACKUP_HDR_ADDR, (uint8_t*) &hdr, sizeof(hdr));

  if(hdr.magic != APP_MAGIC)
    return false;
  if(hdr.image_size == 0)
    return false;
  if(hdr.image_size > APP_MAX_SIZE)
    return false;
  if(hdr.image_size > BACKUP_SLOT_SIZE - BACKUP_HDR_PAD)
    return false;
  return true;
}

bl_backup_status_t bl_backup_current_app(void)
{
  ota_image_hdr_t hdr;

  /* 1. Read current app header */
  read_current_header(&hdr);
  if(hdr.magic != APP_MAGIC)
  {
    USART1_SendString("[BAK] No valid app header\r\n");
    return BL_BACKUP_ERR_READ;
  }
  if(hdr.image_size == 0 || hdr.image_size > APP_MAX_SIZE || hdr.image_size > BACKUP_SLOT_SIZE - BACKUP_HDR_PAD)
  {
    USART1_SendString("[BAK] Bad app size\r\n");
    return BL_BACKUP_ERR_READ;
  }

  /* 2. Verify SOURCE app is valid before copying it. */
  uint32_t src_crc = crc32((const uint8_t*) APP_START_ADDR, hdr.image_size);
  if(src_crc != hdr.crc)
  {
    USART1_SendString("[BAK] Source CRC bad, refusing\r\n");
    return BL_BACKUP_ERR_VERIFY;
  }

  /* 3. Erase only the sectors we will write */
  uint32_t total = BACKUP_HDR_PAD + hdr.image_size;
  uint32_t sectors = sectors_for(total);

  USART1_SendString("[BAK] Erasing ");
  USART1_SendNumber(sectors);
  USART1_SendString(" sectors...\r\n");
  W25Q_Erase(BACKUP_SLOT_ADDR, sectors * W25Q_SECTOR_SIZE);

  /* 4. Write header */
  W25Q_Write(BACKUP_HDR_ADDR, (uint8_t*) &hdr, sizeof(hdr));

  /* 5. Stream body internal flash to W25Q */
  uint8_t buf[W25Q_PAGE_SIZE];
  for(uint32_t off = 0; off < hdr.image_size; off += W25Q_PAGE_SIZE)
  {
    uint32_t n = (hdr.image_size - off >= W25Q_PAGE_SIZE) ? W25Q_PAGE_SIZE : (hdr.image_size - off);
    memcpy(buf, (const void*) (APP_START_ADDR + off), n);
    W25Q_Write(BACKUP_BODY_ADDR + off, buf, n);
  }

  /* 6. Verify header written correctly */
  ota_image_hdr_t hcheck;
  W25Q_FastRead(BACKUP_HDR_ADDR, (uint8_t*) &hcheck, sizeof(hcheck));
  if(hcheck.magic != hdr.magic || hcheck.image_size != hdr.image_size || hcheck.crc != hdr.crc
      || hcheck.version != hdr.version)
  {
    USART1_SendString("[BAK] Header verify FAILED\r\n");
    return BL_BACKUP_ERR_VERIFY;
  }

  /* 7. Verify body by reading back and recomputing CRC */
  uint32_t bak_crc = crc_of_w25q(BACKUP_BODY_ADDR, hdr.image_size);
  if(bak_crc != hdr.crc)
  {
    USART1_SendString("[BAK] Body verify FAILED\r\n");
    return BL_BACKUP_ERR_VERIFY;
  }

  USART1_SendString("[BAK] Backup OK (");
  USART1_SendNumber(hdr.image_size);
  USART1_SendString(" bytes)\r\n");
  return BL_BACKUP_OK;
}

bl_backup_status_t bl_restore_from_backup(void)
{
  ota_image_hdr_t hdr;

  /* 1. Read + validate backup header */
  W25Q_FastRead(BACKUP_HDR_ADDR, (uint8_t*) &hdr, sizeof(hdr));
  if(hdr.magic != APP_MAGIC)
  {
    USART1_SendString("[RST] Backup magic bad\r\n");
    return BL_BACKUP_ERR_CORRUPT;
  }
  if(hdr.image_size == 0 || hdr.image_size > APP_MAX_SIZE || hdr.image_size > BACKUP_SLOT_SIZE - BACKUP_HDR_PAD)
  {
    USART1_SendString("[RST] Backup size bad\r\n");
    return BL_BACKUP_ERR_CORRUPT;
  }

  /* 2. Verify backup CRC BEFORE touching internal flash. */
  USART1_SendString("[RST] Verifying backup...\r\n");
  uint32_t bak_crc = crc_of_w25q(BACKUP_BODY_ADDR, hdr.image_size);
  if(bak_crc != hdr.crc)
  {
    USART1_SendString("[RST] Backup corrupt\r\n");
    return BL_BACKUP_ERR_CORRUPT;
  }

  /* 3. Erase internal header page + app pages under one unlock */
  USART1_SendString("[RST] Erasing internal flash...\r\n");
  HAL_FLASH_Unlock();
  flash_erase_app();
  HAL_FLASH_Lock();

  /* 4. Rewrite app header */
  bl_flash_write_app_header(hdr.magic, hdr.image_size, hdr.crc, hdr.version);

  /* 5. Stream body W25Q to internal flash */
  uint8_t buf[W25Q_PAGE_SIZE];
  HAL_FLASH_Unlock();
  __disable_irq();

  for(uint32_t off = 0; off < hdr.image_size; off += W25Q_PAGE_SIZE)
  {
    uint32_t n = (hdr.image_size - off >= W25Q_PAGE_SIZE) ? W25Q_PAGE_SIZE : (hdr.image_size - off);
    W25Q_FastRead(BACKUP_BODY_ADDR + off, buf, n);

    for(uint32_t i = 0; i < n; i += 4)
    {
      uint32_t word = 0xFFFFFFFF;
      uint32_t b = (n - i >= 4) ? 4 : (n - i);
      memcpy(&word, &buf[i], b);

      if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
      APP_START_ADDR + off + i, (uint64_t) word) != HAL_OK)
      {
        __enable_irq();
        HAL_FLASH_Lock();
        USART1_SendString("[RST] Flash write failed\r\n");
        return BL_BACKUP_ERR_WRITE;
      }
    }
  }
  __enable_irq();
  HAL_FLASH_Lock();

  /* 6. Verify internal flash CRC */
  uint32_t flash_crc = crc32((const uint8_t*) APP_START_ADDR, hdr.image_size);
  if(flash_crc != hdr.crc)
  {
    USART1_SendString("[RST] Restore verify FAILED\r\n");
    return BL_BACKUP_ERR_VERIFY;
  }

  USART1_SendString("[RST] Restore OK (");
  USART1_SendNumber(hdr.image_size);
  USART1_SendString(" bytes)\r\n");
  return BL_BACKUP_OK;
}
