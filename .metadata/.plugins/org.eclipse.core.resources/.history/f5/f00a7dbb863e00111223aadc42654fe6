/*
 * bl_jump.h
 *
 *  Created on: Apr 16, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_BL_JUMP_H_
#define INC_BL_JUMP_H_

#include <stdint.h>

typedef enum
{
  BOOT_SUCCESS = 0,
  MAGIC_ERROR,
  RESET_ERROR,
  SIZE_ERROR,
  CRC_ERROR,
}BootloaderError_t;

void Jump_To_Application(void);
int Bootloader_Is_App_Valid(void);

#endif /* INC_BL_JUMP_H_ */
