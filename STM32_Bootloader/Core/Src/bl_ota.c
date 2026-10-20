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

  hdr.ota_flag = 0;
  hdr.magic    = APP_MAGIC;
  hdr.size     = size;
  hdr.crc      = crc;
  hdr.version  = version;

  HAL_FLASH_Unlock();

  /* flash_erase_header(); */

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

  /* Verify erase actually happened before writing */
  uint32_t check = *(volatile uint32_t*) APP_START_ADDR;
  if(check != 0xFFFFFFFF)
  {
    const char *msg = "[BOTA] ERASE FAILED: 0x";
    while(*msg)
    {
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = *msg++;
    }
    char hx[] = "0123456789ABCDEF";
    for(int k = 28; k >= 0; k -= 4)
    {
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = hx[(check >> k) & 0xF];
    }
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = '\r';
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = '\n';
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
  __disable_irq(); /* ← ADD */

  uint32_t i = 0;
  while(i < len)
  {
    uint32_t word = 0xFFFFFFFF;
    uint32_t bytes = (len - i >= 4) ? 4 : (len - i);
    memcpy(&word, &data[i], bytes);

    HAL_StatusTypeDef st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, ctx->write_addr, (uint64_t) word);

    if(st != HAL_OK)
    {
      /* Direct-register UART poll, no ISR */
      const char *msg = "[BWRITE FAIL] st=";
      while(*msg)
      {
        while(!(USART1->SR & USART_SR_TXE));
        USART1->DR = *msg++;
      }
      char hx[] = "0123456789ABCDEF";
      for(int k = 28; k >= 0; k -= 4)
      {
        while(!(USART1->SR & USART_SR_TXE));
        USART1->DR = hx[((uint32_t) st >> k) & 0xF];
      }
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = '\r';
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = '\n';

      __enable_irq();
      HAL_FLASH_Lock();
      return -2;
    }

    ctx->write_addr += 4;
    i += bytes;
  }

  __enable_irq(); /* ← ADD */
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

  /* ── Compute CRC from ACTUAL flash content ── */
  uint32_t flash_crc = 0xFFFFFFFF;
  const uint8_t *p = (const uint8_t*) APP_START_ADDR;
  for(uint32_t i = 0; i < ctx->image_size; i++)
  {
    flash_crc ^= p[i];
    for(int j = 0; j < 8; j++)
    {
      if(flash_crc & 1)
        flash_crc = (flash_crc >> 1) ^ 0xEDB88320;
      else
        flash_crc >>= 1;
    }
  }
  flash_crc ^= 0xFFFFFFFF;

  /* Debug: show both CRCs */
  {
    const char *m = "[CRC] src=0x";
    while(*m)
    {
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = *m++;
    }
    char hx[] = "0123456789ABCDEF";
    for(int k = 28; k >= 0; k -= 4)
    {
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = hx[(final_crc >> k) & 0xF];
    }
    m = " flash=0x";
    while(*m)
    {
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = *m++;
    }
    for(int k = 28; k >= 0; k -= 4)
    {
      while(!(USART1->SR & USART_SR_TXE));
      USART1->DR = hx[(flash_crc >> k) & 0xF];
    }
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = '\r';
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = '\n';
  }

  if(flash_crc != final_crc)
    return -3;

  /* ── PRE-HDR diagnostic ── */
  volatile uint32_t *sp1 = (volatile uint32_t*) 0x08004400;
  volatile uint32_t *rh1 = (volatile uint32_t*) 0x08004404;
  const char *msg1 = "[PRE-HDR] SP=0x";
  while(*msg1)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = *msg1++;
  }
  char hx[] = "0123456789ABCDEF";
  for(int k = 28; k >= 0; k -= 4)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = hx[(*sp1 >> k) & 0xF];
  }
  msg1 = " RH=0x";
  while(*msg1)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = *msg1++;
  }
  for(int k = 28; k >= 0; k -= 4)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = hx[(*rh1 >> k) & 0xF];
  }
  while(!(USART1->SR & USART_SR_TXE));
  USART1->DR = '\r';
  while(!(USART1->SR & USART_SR_TXE));
  USART1->DR = '\n';

  bl_flash_write_app_header(hdr->magic, ctx->image_size, final_crc, hdr->version);

  /* ── POST-HDR diagnostic ── */
  volatile uint32_t *sp2 = (volatile uint32_t*) 0x08004400;
  volatile uint32_t *rh2 = (volatile uint32_t*) 0x08004404;
  const char *msg2 = "[POST-HDR] SP=0x";
  while(*msg2)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = *msg2++;
  }
  for(int k = 28; k >= 0; k -= 4)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = hx[(*sp2 >> k) & 0xF];
  }
  msg2 = " RH=0x";
  while(*msg2)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = *msg2++;
  }
  for(int k = 28; k >= 0; k -= 4)
  {
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = hx[(*rh2 >> k) & 0xF];
  }
  while(!(USART1->SR & USART_SR_TXE));
  USART1->DR = '\r';
  while(!(USART1->SR & USART_SR_TXE));
  USART1->DR = '\n';

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

