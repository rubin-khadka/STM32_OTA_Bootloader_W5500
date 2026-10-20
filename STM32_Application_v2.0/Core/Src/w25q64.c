/*
 * w25q64.c
 *
 *  Created on: Apr 22, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "w25q64.h"
#include "usart1.h"
#include <string.h>

/* ================= USER CONFIG ================= */
#define W25Q_PAGE_SIZE     256
#define W25Q_SECTOR_SIZE  4096

#define W25Q_SPI hspi2

#define chipSizeinmb      64  // 64megabits

extern SPI_HandleTypeDef W25Q_SPI;

void W25Q_Delay(uint32_t time)
{
  HAL_Delay(time);
}

void csLOW(void)
{
  HAL_GPIO_WritePin(SS_GPIO_Port, SS_Pin, GPIO_PIN_RESET);
}

void csHIGH(void)
{
  HAL_GPIO_WritePin(SS_GPIO_Port, SS_Pin, GPIO_PIN_SET);
}

void SPI_Write(uint8_t *data, uint16_t len)
{
  while(HAL_SPI_GetState(&W25Q_SPI) != HAL_SPI_STATE_READY)
  {
    HAL_Delay(1);
  }
  HAL_StatusTypeDef st = HAL_SPI_Transmit(&W25Q_SPI, data, len, 2000);
  if(st != HAL_OK)
  {
    USART1_SendString("SPI_TX err=");
    USART1_SendNumber(st);
    USART1_SendString("\r\n");
  }
}

void SPI_Read(uint8_t *data, uint16_t len)
{
  /* Flush stale byte + OVR from the previous SPI_Write */
  volatile uint32_t tmp;
  tmp = W25Q_SPI.Instance->DR;
  tmp = W25Q_SPI.Instance->SR;
  (void)tmp;

  uint8_t dummy = 0xFF;
  for(uint16_t i = 0; i < len; i++)
  {
    HAL_SPI_TransmitReceive(&W25Q_SPI, &dummy, &data[i], 1, 5000);
  }
}

/* ================= INTERNAL ================= */

static void write_enable(void)
{
  uint8_t cmd = 0x06;
  csLOW();
  SPI_Write(&cmd, 1);
  csHIGH();
}

static void write_disable(void)
{
  uint8_t cmd = 0x04;
  csLOW();
  SPI_Write(&cmd, 1);
  csHIGH();
}

static uint8_t W25Q_ReadStatus(void)
{
  uint8_t cmd = 0x05;
  uint8_t status;

  csLOW();
  SPI_Write(&cmd, 1);
  SPI_Read(&status, 1);
  csHIGH();

  return status;
}

static void W25Q_WaitForWriteEnd(void)
{
  while(W25Q_ReadStatus() & 0x01)
  {
    W25Q_Delay(1);
  }
}

/* ================= PUBLIC API ================= */

void W25Q_Reset(void)
{
  uint8_t tData[2];
  tData[0] = 0x66;  // enable Reset
  tData[1] = 0x99;  // Reset
  csLOW();
  SPI_Write(tData, 2);
  csHIGH();
  W25Q_Delay(100);
}

uint32_t W25Q_ReadID(void)
{
  uint8_t tx[5] = { 0x9F, 0xFF, 0xFF, 0xFF, 0xFF };
  uint8_t rx[5] = { 0 };

  csLOW();
  HAL_SPI_TransmitReceive(&W25Q_SPI, tx, rx, 5, 5000);
  csHIGH();

  USART1_SendString("ID all: ");
  for(int i = 0; i < 5; i++) { USART1_SendNumber(rx[i]); USART1_SendString(" "); }
  USART1_SendString("\r\n");

  /* Chip responds 2 clocks late — real ID at rx[2], rx[3], rx[4] */
  return ((uint32_t)rx[2] << 16) | ((uint32_t)rx[3] << 8) | rx[4];
}

void W25Q_Erase(uint32_t start_addr, uint32_t size)
{
  uint32_t first_sector = start_addr / W25Q_SECTOR_SIZE;
  uint32_t last_sector  = (start_addr + size - 1) / W25Q_SECTOR_SIZE;

  for(uint32_t sector = first_sector; sector <= last_sector; sector++)
  {
    uint32_t addr = sector * W25Q_SECTOR_SIZE;

    write_enable();

    uint8_t tx[4] = { 0x20,
                      (addr >> 16) & 0xFF,
                      (addr >> 8)  & 0xFF,
                      addr & 0xFF };
    uint8_t rx[4];

    csLOW();
    HAL_SPI_TransmitReceive(&W25Q_SPI, tx, rx, 4, 5000);
    csHIGH();

    W25Q_WaitForWriteEnd();
    write_disable();
  }
}

void W25Q_Write(uint32_t addr, uint8_t *buf, uint16_t len)
{
  uint32_t to_write;
  uint32_t page_remain;

  while(len > 0)
  {
    page_remain = W25Q_PAGE_SIZE - (addr % W25Q_PAGE_SIZE);
    to_write = (len < page_remain) ? len : page_remain;

    write_enable();

    uint8_t tx[4 + 256];
    uint8_t rx[4 + 256];
    uint16_t total = 4 + to_write;

    tx[0] = 0x02;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8)  & 0xFF;
    tx[3] = addr & 0xFF;
    for(uint16_t i = 0; i < to_write; i++) tx[4 + i] = buf[i];

    csLOW();
    HAL_SPI_TransmitReceive(&W25Q_SPI, tx, rx, total, 5000);
    csHIGH();

    W25Q_WaitForWriteEnd();
    write_disable();

    addr += to_write;
    buf += to_write;
    len -= to_write;
  }
}

void W25Q_Read(uint32_t addr, uint8_t *data, uint16_t len)
{
  while(len > 0)
  {
    uint16_t chunk = (len > 256) ? 256 : len;

    uint8_t tx[4 + 256];
    uint8_t rx[4 + 256];
    uint16_t total = 4 + chunk;

    tx[0] = 0x03;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8)  & 0xFF;
    tx[3] = addr & 0xFF;
    for(uint16_t i = 4; i < total; i++) tx[i] = 0xFF;

    csLOW();
    HAL_SPI_TransmitReceive(&W25Q_SPI, tx, rx, total, 5000);
    csHIGH();

    for(uint16_t i = 0; i < chunk; i++) data[i] = rx[4 + i];

    addr += chunk;
    data += chunk;
    len  -= chunk;
  }
}

void W25Q_FastRead(uint32_t addr, uint8_t *data, uint16_t len)
{
  while(len > 0)
  {
    uint16_t chunk = (len > 256) ? 256 : len;

    uint8_t tx[5 + 256];
    uint8_t rx[5 + 256];
    uint16_t total = 5 + chunk;

    tx[0] = 0x0B;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8)  & 0xFF;
    tx[3] = addr & 0xFF;
    tx[4] = 0x00;
    for(uint16_t i = 5; i < total; i++) tx[i] = 0xFF;

    csLOW();
    HAL_SPI_TransmitReceive(&W25Q_SPI, tx, rx, total, 5000);
    csHIGH();

    for(uint16_t i = 0; i < chunk; i++) data[i] = rx[5 + i];

    addr += chunk;
    data += chunk;
    len  -= chunk;
  }
}
