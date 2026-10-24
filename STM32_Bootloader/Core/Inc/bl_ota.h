/*
 * bl_ota.h
 *
 *  Created on: Oct 24, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_BL_OTA_H_
#define INC_BL_OTA_H_

#include <stdint.h>

int check_ota_request(void);

typedef struct
{
  uint32_t write_addr; /* Current flash write address */
  uint32_t total_received; /* Bytes written so far */
  uint32_t image_size; /* Expected firmware size */
  uint32_t expected_crc; /* Expected final CRC */
  uint32_t running_crc; /* CRC accumulator */
} bl_ota_ctx_t;

typedef struct
{
  uint32_t magic;        // OTA image magic
  uint32_t image_size;   // Application binary size ONLY
  uint32_t crc;          // CRC32 of application binary ONLY
  uint32_t version;      // Firmware version
} ota_image_hdr_t;

typedef struct
{
  int (*read)(uint8_t *buf, uint32_t len);
  void (*set_total_size)(uint32_t total_size);
} ota_stream_t;

int bl_ota_run(bl_ota_ctx_t *ctx, ota_stream_t *stream);

uint32_t bl_flash_erase_app(void);
void bl_flash_write_app_header(uint32_t magic, uint32_t size, uint32_t crc, uint32_t version);

void bl_crc_init(uint32_t *crc);
void bl_crc_update(uint32_t *crc, uint8_t *data, uint32_t len);
uint32_t bl_crc_finalize(uint32_t crc);

#endif /* INC_BL_OTA_H_ */
