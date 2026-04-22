/*
 * bl_ota.c
 *
 *  Created on: Apr 22, 2026
 *      Author: Rubin Khadka
 */

#include <flash_layout.h>
#include "main.h"
#include "flash_operations.h"
#include "bl_ota.h"
#include "app_header.h"
#include "string.h"

#define OTA_FLAG_START    1

#define CRC32_POLY 0xEDB88320

void bl_crc_init(uint32_t *crc)
{
  *crc = 0xFFFFFFFF;
}

void bl_crc_update(uint32_t *crc, uint8_t *data, uint32_t len)
{
  for(uint32_t i = 0; i < len; i++)
  {
    *crc ^= data[i];
    for(uint32_t j = 0; j < 8; j++)
    {
      if(*crc & 1)
        *crc = (*crc >> 1) ^ CRC32_POLY;
      else
        *crc >>= 1;
    }
  }
}

uint32_t bl_crc_finalize(uint32_t crc)
{
  return crc ^ 0xFFFFFFFF;
}

int check_ota_request(void)
{
  if(flash_read_ota_flag() == OTA_FLAG_START)
    return 0;
  return 1;
}

uint32_t bl_flash_erase_app(void)
{
  HAL_FLASH_Unlock();
  uint32_t error = flash_erase_app();
  HAL_FLASH_Lock();
  return error;
}

void bl_flash_write_app_header(uint32_t magic, uint32_t size, uint32_t crc, uint32_t version)
{
  app_header_t hdr;

  hdr.ota_flag = 0;  // clear OTA FLAG
  hdr.magic = APP_MAGIC;
  hdr.size = size;
  hdr.crc = crc;
  hdr.version = version;

  HAL_FLASH_Unlock();
  /* Erase header page ONLY */
  flash_erase_header();

  /* Write header word by word */
  uint32_t *p = (uint32_t*) &hdr;
  uint32_t addr = APP_HEADER_ADDR;

  for(uint32_t i = 0; i < sizeof(app_header_t) / 4; i++)
  {
    flash_write_word(addr, p[i]);
    addr += 4;
  }
  HAL_FLASH_Lock();
}

void bl_ota_begin(bl_ota_ctx_t *ctx, uint32_t image_size, uint32_t expected_crc)
{
  bl_flash_erase_app();
  ctx->write_addr = APP_START_ADDR;
  ctx->total_received = 0;
  ctx->image_size = image_size;
  ctx->expected_crc = expected_crc;
  bl_crc_init(&ctx->running_crc);
}

int bl_ota_write_chunk(bl_ota_ctx_t *ctx, uint8_t *data, uint32_t len)
{
  if((ctx->total_received + len) > ctx->image_size)
    return -1;

  /* Update CRC ONLY on real data bytes */
  bl_crc_update(&ctx->running_crc, data, len);

  /* Write data to flash, handling last unaligned word safely */
  HAL_FLASH_Unlock();
  uint32_t i = 0;
  while(i < len)
  {
    uint32_t word = 0xFFFFFFFF;
    uint32_t bytes = (len - i >= 4) ? 4 : (len - i);

    memcpy(&word, &data[i], bytes);

    flash_write_word(ctx->write_addr, word);
    ctx->write_addr += 4;
    i += bytes;
  }
  HAL_FLASH_Lock();
  ctx->total_received += len;
  return 0;
}

int bl_ota_finalize(bl_ota_ctx_t *ctx, ota_image_hdr_t *hdr)
{
  if(ctx->total_received != ctx->image_size)
    return -1;

  uint32_t final_crc = bl_crc_finalize(ctx->running_crc);

  if(final_crc != ctx->expected_crc)
  {
    /* OTA failed → stay in bootloader */
    return -2;
  }

  /* Commit application */
  bl_flash_write_app_header(hdr->magic, ctx->image_size, final_crc, hdr->version);
  return 0;
}

int bl_ota_run(bl_ota_ctx_t *ctx, ota_stream_t *stream)
{
  ota_image_hdr_t hdr;

  /* Read OTA header */
  if(stream->read((uint8_t*) &hdr, sizeof(hdr)) != 0)
    return -1;

  /* Validate header */
  if(hdr.magic != APP_MAGIC)
    return -2;

  if(hdr.image_size == 0 || hdr.image_size > APP_MAX_SIZE)
    return -3;

  /* Calculate total OTA image size */
  uint32_t total_size = sizeof(ota_image_hdr_t) + hdr.image_size;

  /* Inform stream about bounds */
  if(stream->set_total_size)
    stream->set_total_size(total_size);

  /* Start OTA */
  bl_ota_begin(ctx, hdr.image_size, hdr.crc);

  /* Receive firmware payload */
  uint32_t remaining = hdr.image_size;
  uint8_t buf[256];

  while(remaining)
  {
    uint32_t len = (remaining > sizeof(buf)) ? sizeof(buf) : remaining;

    if(stream->read(buf, len) != 0)
      return -4;

    if(bl_ota_write_chunk(ctx, buf, len) != 0)
      return -5;

    remaining -= len;
  }

  /* Finalize OTA */
  if(bl_ota_finalize(ctx, &hdr) != 0)
    return -6;

  return 0;
}

