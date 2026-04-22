/*
 * w25q64.c
 *
 *  Created on: Apr 22, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "w25q64.h"
#include <string.h>

/* ================= USER CONFIG ================= */
#define W25Q_PAGE_SIZE     256
#define W25Q_SECTOR_SIZE  4096

#define W25Q_SPI hspi2

#define chipSizeinmb			64	// 64megabits

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
  HAL_SPI_Transmit(&W25Q_SPI, data, len, 2000);
}

void SPI_Read(uint8_t *data, uint16_t len)
{
  uint8_t dummy = 0x00;
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
  uint8_t tData = 0x9F;  // Read JEDEC ID
  uint8_t rData[3];
  csLOW();
  SPI_Write(&tData, 1);
  SPI_Read(rData, 3);
  csHIGH();
  return ((rData[0] << 16) | (rData[1] << 8) | rData[2]);
}

void W25Q_Erase(uint32_t start_addr, uint32_t size)
{
  uint32_t first_sector = start_addr / W25Q_SECTOR_SIZE;
  uint32_t last_sector = (start_addr + size - 1) / W25Q_SECTOR_SIZE;

  for(uint32_t sector = first_sector; sector <= last_sector; sector++)
  {
    uint32_t addr = sector * W25Q_SECTOR_SIZE;
    uint8_t cmd[5];

    write_enable();

    if(chipSizeinmb < 256)
    {
      cmd[0] = 0x20; // Sector Erase (3-byte address)
      cmd[1] = (addr >> 16) & 0xFF;
      cmd[2] = (addr >> 8) & 0xFF;
      cmd[3] = addr & 0xFF;

      csLOW();
      SPI_Write(cmd, 4);
      csHIGH();
    }
    else
    {
      cmd[0] = 0x21; // Sector Erase (4-byte address)
      cmd[1] = (addr >> 24) & 0xFF;
      cmd[2] = (addr >> 16) & 0xFF;
      cmd[3] = (addr >> 8) & 0xFF;
      cmd[4] = addr & 0xFF;

      csLOW();
      SPI_Write(cmd, 5);
      csHIGH();
    }

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

    uint8_t cmd[5];
    if(chipSizeinmb < 256)
    {
      cmd[0] = 0x02; // Page Program
      cmd[1] = (addr >> 16) & 0xFF;
      cmd[2] = (addr >> 8) & 0xFF;
      cmd[3] = addr & 0xFF;

      csLOW();
      SPI_Write(cmd, 4);
    }
    else
    {
      cmd[0] = 0x12; // Page Program (4-byte)
      cmd[1] = (addr >> 24) & 0xFF;
      cmd[2] = (addr >> 16) & 0xFF;
      cmd[3] = (addr >> 8) & 0xFF;
      cmd[4] = addr & 0xFF;

      csLOW();
      SPI_Write(cmd, 5);
    }

    SPI_Write(buf, to_write);
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
  uint8_t cmd[6];   // opcode + max 4 address bytes + dummy (optional)
  uint16_t idx;

  idx = 0;

  if(chipSizeinmb < 256)
  {
    cmd[idx++] = 0x03;   // READ (3-byte address)
    cmd[idx++] = (addr >> 16) & 0xFF;
    cmd[idx++] = (addr >> 8) & 0xFF;
    cmd[idx++] = addr & 0xFF;
  }
  else
  {
    cmd[idx++] = 0x13;   // READ (4-byte address)
    cmd[idx++] = (addr >> 24) & 0xFF;
    cmd[idx++] = (addr >> 16) & 0xFF;
    cmd[idx++] = (addr >> 8) & 0xFF;
    cmd[idx++] = addr & 0xFF;
  }

  csLOW();
  SPI_Write(cmd, idx);
  SPI_Read(data, len);   // continuous read is allowed
  csHIGH();

}

void W25Q_FastRead(uint32_t addr, uint8_t *data, uint16_t len)
{
  uint8_t cmd[7];
  uint16_t idx;

  idx = 0;

  if(chipSizeinmb < 256)
  {
    cmd[idx++] = 0x0B;   // FAST READ (3-byte addr)
    cmd[idx++] = (addr >> 16) & 0xFF;
    cmd[idx++] = (addr >> 8) & 0xFF;
    cmd[idx++] = addr & 0xFF;
    cmd[idx++] = 0x00;   // dummy
  }
  else
  {
    cmd[idx++] = 0x0C;   // FAST READ (4-byte addr)
    cmd[idx++] = (addr >> 24) & 0xFF;
    cmd[idx++] = (addr >> 16) & 0xFF;
    cmd[idx++] = (addr >> 8) & 0xFF;
    cmd[idx++] = addr & 0xFF;
    cmd[idx++] = 0x00;   // dummy
  }

  csLOW();
  SPI_Write(cmd, idx);
  SPI_Read(data, len);
  csHIGH();
}
