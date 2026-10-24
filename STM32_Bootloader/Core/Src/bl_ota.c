/*
 * bl_ota.c
 *
 *  Created on: Apr 22, 2026
 *      Author: Rubin Khadka
 */

#include <bl_ota.h>
#include <flash_layout.h>
#include "main.h"
#include "flash_operations.h"
#include "app_header.h"
#include <string.h>

#define OTA_FLAG_START    1
#define CRC32_POLY        0xEDB88320

/* ────────────────────────────────────────────────────────────── */
/* compact logging helpers (direct-register UART, no ISR)        */
/* ────────────────────────────────────────────────────────────── */

static void log_str(const char *s)
{
  while(*s)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = *s++;
  }
}

static void log_hex32(uint32_t v)
{
  static const char hx[] = "0123456789ABCDEF";
  for(int k = 28; k >= 0; k -= 4)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = hx[(v >> k) & 0xF];
  }
}

static void log_nl(void)
{
  while(!(USART1->SR & USART_SR_TXE));
  USART1->DR = '\r';
  while(!(USART1->SR & USART_SR_TXE));
  USART1->DR = '\n';
}

/* ────────────────────────────────────────────────────────────── */
/* CRC32                                                         */
/* ────────────────────────────────────────────────────────────── */

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

/* ────────────────────────────────────────────────────────────── */
/* flash helpers                                                 */
/* ────────────────────────────────────────────────────────────── */

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

  hdr.ota_flag = 0;
  hdr.magic = APP_MAGIC;
  hdr.size = size;
  hdr.crc = crc;
  hdr.version = version;

  HAL_FLASH_Unlock();

  uint32_t *p = (uint32_t*) &hdr;
  uint32_t addr = APP_HEADER_ADDR;

  for(uint32_t i = 0; i < sizeof(app_header_t) / 4; i++)
  {
    flash_write_word(addr, p[i]);
    addr += 4;
  }
  HAL_FLASH_Lock();
}

/* ────────────────────────────────────────────────────────────── */
/* OTA core                                                      */
/* ────────────────────────────────────────────────────────────── */

void bl_ota_begin(bl_ota_ctx_t *ctx, uint32_t image_size, uint32_t expected_crc)
{
  bl_flash_erase_app();

  /* Sanity: erase must have produced 0xFFFFFFFF at the app start */
  if(*(volatile uint32_t*) APP_START_ADDR != 0xFFFFFFFF)
  {
    log_str("[BOTA] ERASE FAILED: 0x");
    log_hex32(*(volatile uint32_t*) APP_START_ADDR);
    log_nl();
  }

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

  bl_crc_update(&ctx->running_crc, data, len);

  HAL_FLASH_Unlock();
  __disable_irq();

  uint32_t i = 0;
  while(i < len)
  {
    uint32_t word = 0xFFFFFFFF;
    uint32_t bytes = (len - i >= 4) ? 4 : (len - i);
    memcpy(&word, &data[i], bytes);

    HAL_StatusTypeDef st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, ctx->write_addr, (uint64_t) word);
    if(st != HAL_OK)
    {
      log_str("[BWRITE FAIL] st=");
      log_hex32((uint32_t) st);
      log_nl();

      __enable_irq();
      HAL_FLASH_Lock();
      return -2;
    }

    ctx->write_addr += 4;
    i += bytes;
  }

  __enable_irq();
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
    return -2;

  /* Recompute CRC from actual flash (memory-mapped) using the same
   * incremental routine — avoids a duplicate CRC loop. */
  uint32_t flash_crc = 0xFFFFFFFF;
  bl_crc_update(&flash_crc, (uint8_t*) APP_START_ADDR, ctx->image_size);
  flash_crc = bl_crc_finalize(flash_crc);

  log_str("[CRC] src=0x");
  log_hex32(final_crc);
  log_str(" flash=0x");
  log_hex32(flash_crc);
  log_nl();

  if(flash_crc != final_crc)
    return -3;

  bl_flash_write_app_header(hdr->magic, ctx->image_size, final_crc, hdr->version);
  return 0;
}

int bl_ota_run(bl_ota_ctx_t *ctx, ota_stream_t *stream)
{
  ota_image_hdr_t hdr;

  if(stream->read((uint8_t*) &hdr, sizeof(hdr)) != 0)
    return -1;

  if(hdr.magic != APP_MAGIC)
    return -2;

  if(hdr.image_size == 0 || hdr.image_size > APP_MAX_SIZE)
    return -3;

  uint32_t total_size = sizeof(ota_image_hdr_t) + hdr.image_size;

  if(stream->set_total_size)
    stream->set_total_size(total_size);

  bl_ota_begin(ctx, hdr.image_size, hdr.crc);

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

  if(bl_ota_finalize(ctx, &hdr) != 0)
    return -6;

  return 0;
}

