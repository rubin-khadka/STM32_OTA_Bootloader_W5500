/*
 * wizchip_port.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "wizchip_conf.h"
#include "string.h"
#include "socket.h"
#include "usart1.h"

// W5500 Initialization Return Codes
typedef enum
{
  W5500_OK = 0,
  W5500_ERR_INIT = -1,
  W5500_ERR_COMM = -2,
  W5500_ERR_LINK_DOWN = 3
} W5500_Status_t;

#define W5500_SPI hspi1

wiz_NetInfo netInfo = { .mac = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 }, .ip = { 192, 168, 1, 10 }, .sn =
    { 255, 255, 255, 0 }, .gw = { 0, 0, 0, 0 }, .dns = { 0, 0, 0, 0 }, .dhcp = NETINFO_STATIC };

#define W5500_CS_LOW()     HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define W5500_CS_HIGH()    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)
#define W5500_RST_LOW()    HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_RESET)
#define W5500_RST_HIGH()   HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_SET)

extern SPI_HandleTypeDef W5500_SPI;

// SPI transmit/receive
void W5500_Select(void)
{
  W5500_CS_LOW();
}
void W5500_Unselect(void)
{
  W5500_CS_HIGH();
}

uint8_t W5500_ReadByte(void)
{
  uint8_t rx;
  uint8_t tx = 0xFF;
  HAL_SPI_TransmitReceive(&W5500_SPI, &tx, &rx, 1, HAL_MAX_DELAY);
  return rx;
}

void W5500_WriteByte(uint8_t byte)
{
  HAL_SPI_Transmit(&W5500_SPI, &byte, 1, HAL_MAX_DELAY);
}

int W5500_Init(void)
{
  uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2, 2, 2 } };

  /***** Reset Sequence  *****/
  W5500_RST_LOW();
  HAL_Delay(50);
  W5500_RST_HIGH();
  HAL_Delay(200);

  /***** Register callback  *****/
  reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
  reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);

  /***** Initialize the chip  *****/
  if(ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1)
  {
    USART1_SendString("Error while initializing WIZCHIP\r\n");
    return W5500_ERR_INIT;
  }
  USART1_SendString("WIZCHIP Initialized\r\n");

  /***** Check communication by reading Version  *****/
  uint8_t ver = getVERSIONR();
  if(ver != 0x04)
  {
    USART1_SendString("Error Communicating with W5500 Version: ");
    USART1_SendHex(ver);
    USART1_SendString("\r\n");
    return W5500_ERR_COMM;
  }
  USART1_SendString("Checking Link Status..\r\n");

  /***** Check Link Status  *****/
  uint8_t link = PHY_LINK_OFF;
  uint8_t retries = 10;
  while((link != PHY_LINK_ON) && (retries > 0))
  {
    ctlwizchip(CW_GET_PHYLINK, &link);
    if(link == PHY_LINK_ON)
      USART1_SendString("Link: UP\r\n");
    else
    {
      USART1_SendString("Link: DOWN Retrying: ");
      USART1_SendNumber(10 - retries);
      USART1_SendString("\r\n");
    }
    retries--;
    HAL_Delay(500);
  }
  if(link != PHY_LINK_ON)
  {
    USART1_SendString("Link is Down, please reconnect and retry\r\n");
    USART1_SendString("Exiting Setup..\r\n");
    return W5500_ERR_LINK_DOWN;
  }

  /***** Configure Static IP  *****/
  USART1_SendString("Using Static IP..\r\n");
  ctlnetwork(CN_SET_NETINFO, (void*) &netInfo);

  /***** Print assigned IP on the console  *****/
  wiz_NetInfo tmpInfo;
  ctlnetwork(CN_GET_NETINFO, &tmpInfo);

  USART1_SendString("IP: ");
  USART1_SendNumber(tmpInfo.ip[0]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.ip[1]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.ip[2]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.ip[3]);
  USART1_SendString("\r\n");

  USART1_SendString("SUBNET: ");
  USART1_SendNumber(tmpInfo.sn[0]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.sn[1]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.sn[2]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.sn[3]);
  USART1_SendString("\r\n");

  USART1_SendString("GATEWAY: ");
  USART1_SendNumber(tmpInfo.gw[0]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.gw[1]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.gw[2]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.gw[3]);
  USART1_SendString("\r\n");

  USART1_SendString("DNS: ");
  USART1_SendNumber(tmpInfo.dns[0]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.dns[1]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.dns[2]);
  USART1_SendString(".");
  USART1_SendNumber(tmpInfo.dns[3]);
  USART1_SendString("\r\n");

  return W5500_OK;
}
