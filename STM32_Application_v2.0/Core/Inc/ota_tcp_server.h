/*
 * ota_tcp_server.h
 *
 *  Created on: Apr 23, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_OTA_TCP_SERVER_H_
#define INC_OTA_TCP_SERVER_H_

#define OTA_STAGE_IDLE        0
#define OTA_STAGE_CONNECTING  1
#define OTA_STAGE_RECEIVING   2
#define OTA_STAGE_WRITING     3
#define OTA_STAGE_VERIFYING   4
#define OTA_STAGE_COMPLETE    5
#define OTA_STAGE_ERROR       6

extern volatile uint8_t ota_active;
extern volatile uint8_t ota_status_stage;
extern volatile uint32_t ota_progress_current;
extern volatile uint32_t ota_progress_total;

int OTA_Client_Init(void);
void OTA_Client_Task(void);
void ota_start (void);
void ota_stop(void);
void test_tcp_communication(void);

#endif /* INC_OTA_TCP_SERVER_H_ */
