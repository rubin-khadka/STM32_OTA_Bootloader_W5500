/*
 * ota_tcp_server.h
 *
 *  Created on: Apr 23, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_OTA_TCP_SERVER_H_
#define INC_OTA_TCP_SERVER_H_

int OTA_Client_Init(void);
void OTA_Client_Task(void);
void ota_start (void);
void ota_stop(void);
void test_tcp_communication(void);

#endif /* INC_OTA_TCP_SERVER_H_ */
