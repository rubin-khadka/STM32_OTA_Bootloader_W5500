/*
 * app_ota.h
 *
 *  Created on: Apr 23, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_APP_OTA_H_
#define INC_APP_OTA_H_

#define APP_MAGIC 0xABCDEFAB

typedef struct
{
  uint32_t magic;        // OTA image magic
  uint32_t image_size;   // Application binary size ONLY
  uint32_t crc;          // CRC32 of application binary ONLY
  uint32_t version;      // Firmware version
} ota_image_hdr_t;

void enable_ota_request(void);

#endif /* INC_APP_OTA_H_ */
