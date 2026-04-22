/*
 * crc32.c
 *
 *  Created on: Apr 17, 2026
 *      Author: Rubin Khadka
 */

#include "crc32.h"

uint32_t crc32(const uint8_t *data, uint32_t length)
{
  uint32_t crc = 0xFFFFFFFF;

  for(uint32_t i = 0; i < length; i++)
  {
    crc ^= data[i];
    for(uint8_t j = 0; j < 8; j++)
    {
      if(crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc >>= 1;
    }
  }

  return crc ^ 0xFFFFFFFF;
}

