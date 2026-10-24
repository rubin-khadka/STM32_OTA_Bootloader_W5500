/*
 * bl_backup.h
 *
 *  Created on: Oct 24, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_BL_BACKUP_H_
#define INC_BL_BACKUP_H_

#include <stdint.h>
#include <stdbool.h>

/* Slot layout */
#define BACKUP_SLOT_ADDR   0x7E0000UL
#define BACKUP_SLOT_SIZE   0x20000UL                     /* 128 KB (2 x 64 KB block) */
#define BACKUP_HDR_ADDR    BACKUP_SLOT_ADDR
#define BACKUP_BODY_ADDR   (BACKUP_SLOT_ADDR + 0x100UL)  /* next page */

// Backup status code
typedef enum {
    BL_BACKUP_OK = 0,
    BL_BACKUP_ERR_READ,
    BL_BACKUP_ERR_WRITE,
    BL_BACKUP_ERR_VERIFY,
    BL_BACKUP_ERR_CORRUPT
} bl_backup_status_t;

// Backup Current Application
bl_backup_status_t bl_backup_current_app(void);

// Restore Old Backup from External Flash
bl_backup_status_t bl_restore_from_backup(void);

// Check if Backup Available
bool bl_backup_available(void);

#endif /* INC_BL_BACKUP_H_ */
