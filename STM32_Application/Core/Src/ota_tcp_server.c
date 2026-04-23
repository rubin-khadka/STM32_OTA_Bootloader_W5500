/*
 * ota_tcp_server.c
 *
 *  Created on: Apr 23, 2026
 *      Author: Rubin Khadka
 */

/*
 * OTA_TCP_Server.c
 *
 * Raw TCP OTA upload server for W5500 with W25Q64 external flash
 * Adapted from ESP8266 OTA logic
 */

#include "wizchip_conf.h"
#include "wizchip_port.h"
#include "socket.h"
#include "string.h"
#include "stdlib.h"
#include "main.h"
#include "flash_layout.h"
#include "flash_operations.h"
#include "app_ota.h"
#include "usart1.h"
#include "w25q64.h"

/* ================= CONFIG ================= */
#define SERVER_IP           {192, 168, 1, 2}
#define SERVER_PORT         5678

#define CLIENT_SOCKET       0
#define CLIENT_PORT         1234

#define OTA_RX_BUF_SIZE     512
#define OTA_ACK_CHUNK       512
#define OTA_FLASH_OFFSET    0                   // Start of W25Q64

/* ========================================== */
static uint8_t server_ip[4];
uint8_t ota_active = 0;

/* OTA state variables */
static uint8_t rx_buf[OTA_RX_BUF_SIZE];
static uint32_t flash_offset = OTA_FLASH_OFFSET;
static uint32_t bytes_received = 0;
static uint8_t flash_erased = 0;
static ota_image_hdr_t ota_hdr;
static uint8_t header_received = 0;
static uint8_t ota_session_active = 0;
static uint8_t request_sent = 0;

static uint32_t bytes_in_current_chunk = 0;
static uint32_t total_received = 0;
static uint32_t last_progress_tick = 0;
static uint32_t last_bytes_received = 0;

#define OTA_HEADER_SIZE sizeof(ota_image_hdr_t)

/* Debug macros */
#define DEBUG_STR(str) USART1_SendString(str)
#define DEBUG_STR_LN(str) USART1_SendString(str); USART1_SendString("\r\n")
#define DEBUG_NUM(num) USART1_SendNumber(num)
#define DEBUG_HEX(val) USART1_SendHex(val)

/* Forward declarations */
static void ota_process_rx(uint8_t *data, uint16_t len);
static void ota_finalize(void);
static void ota_error(void);

/* ================= W25Q64 OTA HELPERS ================= */

static void erase_ota_flash(void)
{
  DEBUG_STR_LN("Erasing W25Q64 OTA area...");
  W25Q_Erase(OTA_FLASH_OFFSET, APP_MAX_SIZE);
  HAL_Delay(100);  // Wait for erase to start
  DEBUG_STR_LN("Erase complete");
}

static uint8_t ota_write(uint32_t addr, uint8_t *buf, size_t len)
{
  uint8_t verify_buf[512];

  if(len == 0)
    return 1;

  DEBUG_STR("Writing to W25Q64 at 0x");
  DEBUG_NUM(addr);
  DEBUG_STR(" (");
  DEBUG_NUM(len);
  DEBUG_STR_LN(" bytes)");

  // Verify area is erased before writing
  W25Q_FastRead(addr, verify_buf, len);
  for(size_t i = 0; i < len; i++)
  {
    if(verify_buf[i] != 0xFF)
    {
      DEBUG_STR_LN("ERROR: Flash not erased before write!");
      DEBUG_STR("Address 0x");
      DEBUG_NUM(addr + i);
      DEBUG_STR(" has value 0x");
      DEBUG_HEX(verify_buf[i]);
      DEBUG_STR_LN(" (should be 0xFF)");
      ota_error();
      return 0;
    }
  }

  // Write to W25Q64
  W25Q_Write(addr, buf, len);
  HAL_Delay(1);

  // Verify the write
  W25Q_FastRead(addr, verify_buf, len);

  if(memcmp(buf, verify_buf, len) != 0)
  {
    DEBUG_STR_LN("!!! FLASH WRITE VERIFY FAILED !!!");

    // Find first mismatch
    for(size_t k = 0; k < len; k++)
    {
      if(buf[k] != verify_buf[k])
      {
        DEBUG_STR("First mismatch at offset ");
        DEBUG_NUM(k);
        DEBUG_STR(": wrote 0x");
        DEBUG_HEX(buf[k]);
        DEBUG_STR(", read 0x");
        DEBUG_HEX(verify_buf[k]);
        DEBUG_STR_LN("");
        break;
      }
    }
    ota_error();
    return 0;
  }

  DEBUG_STR_LN("✓ Write verified");
  return 1;
}

/* ================= OTA PROCESSING ================= */

static void ota_process_rx(uint8_t *data, uint16_t len)
{
  uint16_t offset = 0;

  if(len == 0)
    return;

  DEBUG_STR("RX: ");
  DEBUG_NUM(len);
  DEBUG_STR_LN(" bytes");

  /* Erase flash once at start */
  if(!flash_erased)
  {
    erase_ota_flash();
    flash_erased = 1;
  }

  /* ---------------- Parse OTA header ---------------- */
  if(!header_received)
  {
    uint16_t hdr_bytes_needed = OTA_HEADER_SIZE - bytes_received;

    if(len >= hdr_bytes_needed)
    {
      // Complete header received
      memcpy(((uint8_t*) &ota_hdr) + bytes_received, data, hdr_bytes_needed);

      // Write header to flash
      if(!ota_write(flash_offset, data, hdr_bytes_needed))
      {
        return;  // Write failed
      }

      flash_offset += hdr_bytes_needed;
      offset += hdr_bytes_needed;
      bytes_received += hdr_bytes_needed;
      header_received = 1;

      DEBUG_STR("OTA header received: magic=0x");
      for(int i = 0; i < 4; i++)
      {
        DEBUG_HEX(((uint8_t* )&ota_hdr.magic)[i]);
      }
      DEBUG_STR(", size=");
      DEBUG_NUM(ota_hdr.image_size);
      DEBUG_STR(", version=");
      DEBUG_NUM(ota_hdr.version);
      DEBUG_STR_LN("");

      // Validate magic number
      if(ota_hdr.magic != APP_MAGIC)
      {
        DEBUG_STR_LN("ERROR: Invalid magic number!");
        DEBUG_STR("Expected: 0x");
        DEBUG_NUM(APP_MAGIC);
        DEBUG_STR(", Got: 0x");
        DEBUG_NUM(ota_hdr.magic);
        DEBUG_STR_LN("");
        ota_error();
        return;
      }
    }
    else
    {
      // Partial header received
      memcpy(((uint8_t*) &ota_hdr) + bytes_received, data, len);

      // Write partial header to flash
      if(!ota_write(flash_offset, data, len))
      {
        return;
      }

      flash_offset += len;
      bytes_received += len;
      DEBUG_STR("Partial header: ");
      DEBUG_NUM(bytes_received);
      DEBUG_STR("/");
      DEBUG_NUM(OTA_HEADER_SIZE);
      DEBUG_STR_LN(" bytes");
      return;
    }
  }

  /* ---------------- Write firmware body ---------------- */
  uint32_t fw_bytes_received = bytes_received - OTA_HEADER_SIZE;
  uint32_t fw_bytes_to_write = len - offset;

  if(fw_bytes_received + fw_bytes_to_write > ota_hdr.image_size)
  {
    fw_bytes_to_write = ota_hdr.image_size - fw_bytes_received;
  }

  if(fw_bytes_to_write > 0)
  {
    if(!ota_write(flash_offset, &data[offset], fw_bytes_to_write))
    {
      return;  // Write failed
    }

    flash_offset += fw_bytes_to_write;
    bytes_received += fw_bytes_to_write;
  }

  bytes_in_current_chunk += len;
  total_received = bytes_received;

  DEBUG_STR("Progress: ");
  DEBUG_NUM(bytes_received);
  DEBUG_STR("/");
  DEBUG_NUM(ota_hdr.image_size + OTA_HEADER_SIZE);
  DEBUG_STR_LN("");

  /* ---------------- Send ACK every chunk ---------------- */
  if(bytes_in_current_chunk >= OTA_ACK_CHUNK)
  {
    DEBUG_STR_LN("Sending ACK");
    send(CLIENT_SOCKET, (uint8_t*) "A", 1);
    bytes_in_current_chunk = 0;
  }

  /* ---------------- Completion check ---------------- */
  if(header_received && bytes_received >= ota_hdr.image_size + OTA_HEADER_SIZE)
  {
    DEBUG_STR_LN("\n========================================");
    DEBUG_STR("OTA upload complete! Total bytes: ");
    DEBUG_NUM(bytes_received);
    DEBUG_STR_LN("");
    DEBUG_STR("Firmware size: ");
    DEBUG_NUM(ota_hdr.image_size);
    DEBUG_STR_LN("");
    DEBUG_STR_LN("========================================\n");
    ota_finalize();
  }
}

/* ================= TCP CLIENT INITIALIZATION ================= */

int8_t OTA_Client_Init(void)
{
  int8_t ret;
  uint8_t status;

  DEBUG_STR_LN("Initializing W5500 TCP client...");
  HAL_Delay(20);

  // Force clean state
  close(CLIENT_SOCKET);
  while(getSn_SR(CLIENT_SOCKET) != SOCK_CLOSED)
  {
    HAL_Delay(10);
  }

  // Create socket
  ret = socket(CLIENT_SOCKET, Sn_MR_TCP, CLIENT_PORT, 0);
  if(ret != CLIENT_SOCKET)
  {
    DEBUG_STR("Socket creation failed, ret=");
    DEBUG_NUM(ret);
    DEBUG_STR_LN("");
    return -1;
  }

  HAL_Delay(50);

  // Wait for INIT state
  do
  {
    status = getSn_SR(CLIENT_SOCKET);
  }
  while(status != SOCK_INIT);

  DEBUG_STR("Socket status: 0x");
  DEBUG_HEX(status);
  DEBUG_STR_LN("");

  // Set static server IP
  uint8_t static_ip[4] = SERVER_IP;
  memcpy(server_ip, static_ip, 4);

  DEBUG_STR("Connecting to ");
  DEBUG_NUM(server_ip[0]);
  DEBUG_STR(".");
  DEBUG_NUM(server_ip[1]);
  DEBUG_STR(".");
  DEBUG_NUM(server_ip[2]);
  DEBUG_STR(".");
  DEBUG_NUM(server_ip[3]);
  DEBUG_STR(":");
  DEBUG_NUM(SERVER_PORT);
  DEBUG_STR_LN(" ...");

  // Connect with retries
  int retries = 5;
  do
  {
    ret = connect(CLIENT_SOCKET, server_ip, SERVER_PORT);
    status = getSn_SR(CLIENT_SOCKET);

    uint32_t max_wait_ms = 5000;
    uint32_t start = HAL_GetTick();
    while(((HAL_GetTick() - start) < max_wait_ms) && (status != SOCK_ESTABLISHED))
    {
      status = getSn_SR(CLIENT_SOCKET);
      if(status == SOCK_ESTABLISHED)
        break;
    }
    retries--;
  }
  while((retries > 0) && (status != SOCK_ESTABLISHED));

  if(status != SOCK_ESTABLISHED)
  {
    DEBUG_STR_LN("Connection timeout!");
    close(CLIENT_SOCKET);
    return -2;
  }

  DEBUG_STR_LN("Connected to server successfully!");
  return 0;
}

/* ================= MAIN OTA TASK ================= */

void OTA_Client_Task(void)
{
  int32_t len;
  uint8_t status = getSn_SR(CLIENT_SOCKET);

  if(!ota_active)
    return;

  switch(status)
  {
    case SOCK_ESTABLISHED:
      // Send request once
      if(!request_sent)
      {
        const char req[] = "GET firmware\r\n";
        len = send(CLIENT_SOCKET, (uint8_t*) req, strlen(req));
        if(len > 0)
        {
          DEBUG_STR_LN("Sent OTA request");
          request_sent = 1;
          last_progress_tick = HAL_GetTick();
        }
        else
        {
          DEBUG_STR_LN("Send error");
        }
      }

      // Check for received data
      uint16_t rx_size = getSn_RX_RSR(CLIENT_SOCKET);
      if(rx_size > 0)
      {
        if(rx_size > OTA_RX_BUF_SIZE)
          rx_size = OTA_RX_BUF_SIZE;

        len = recv(CLIENT_SOCKET, rx_buf, rx_size);
        if(len > 0)
        {
          ota_process_rx(rx_buf, len);
          last_progress_tick = HAL_GetTick();
          last_bytes_received = bytes_received;
        }
      }
      break;

    case SOCK_CLOSE_WAIT:
      // Drain remaining data
      len = recv(CLIENT_SOCKET, rx_buf, sizeof(rx_buf));
      if(len > 0)
      {
        ota_process_rx(rx_buf, len);
      }
      DEBUG_STR("Close-wait, total: ");
      DEBUG_NUM(total_received);
      DEBUG_STR_LN(" bytes")
      ;
      break;

    case SOCK_CLOSED:
      if(ota_session_active)
      {
        DEBUG_STR("Connection closed, total: ");
        DEBUG_NUM(total_received);
        DEBUG_STR_LN(" bytes");
        ota_session_active = 0;
      }
      break;

    default:
      break;
  }

  // Stall detection (no progress for 10 seconds)
  uint32_t now = HAL_GetTick();
  if(ota_active && header_received && (now - last_progress_tick > 10000))
  {
    if(bytes_received == last_bytes_received && bytes_received > 0)
    {
      DEBUG_STR_LN("OTA stalled! No progress for 10 seconds");
      DEBUG_STR("Received: ");
      DEBUG_NUM(bytes_received);
      DEBUG_STR("/");
      DEBUG_NUM(ota_hdr.image_size + OTA_HEADER_SIZE);
      DEBUG_STR_LN("");
      ota_error();
    }
    last_bytes_received = bytes_received;
    last_progress_tick = now;
  }
}

/* ================= OTA START ================= */

void ota_start(void)
{
  // Initialize W25Q64 first
  DEBUG_STR_LN("\n========================================");
  DEBUG_STR_LN("Starting W5500 OTA Client");
  DEBUG_STR_LN("========================================\n");

  // Check W25Q64
  DEBUG_STR_LN("Checking W25Q64...");
  W25Q_Reset();
  HAL_Delay(100);

  uint32_t flash_id = W25Q_ReadID();
  DEBUG_STR("Flash ID: 0x");
  DEBUG_NUM(flash_id);
  DEBUG_STR_LN("");

  if(flash_id != 0xEF4017)
  {
    DEBUG_STR_LN("ERROR: Wrong flash ID! Expected 0xEF4017");
    return;
  }
  DEBUG_STR_LN("W25Q64 detected OK\n");

  // Initialize W5500
  if(W5500_Init() != 0)
  {
    DEBUG_STR_LN("W5500 initialization failed!");
    return;
  }

  HAL_Delay(2000);  // Let PHY stabilize

  // Connect to server
  uint8_t retries = 5;
  do
  {
    if(OTA_Client_Init() == 0)
    {
      ota_active = 1;
      ota_session_active = 1;

      // Reset OTA state variables
      flash_offset = OTA_FLASH_OFFSET;
      bytes_received = 0;
      header_received = 0;
      flash_erased = 0;
      bytes_in_current_chunk = 0;
      total_received = 0;
      request_sent = 0;
      last_progress_tick = HAL_GetTick();
      last_bytes_received = 0;

      DEBUG_STR_LN("\nOTA active, waiting for firmware...\n");
      break;
    }
    HAL_Delay(1000);
    retries--;
  }
  while(retries > 0);

  if(!ota_active)
  {
    DEBUG_STR_LN("Failed to connect to server!");
  }
}

/* ================= OTA FINALIZE & ERROR ================= */

static void ota_finalize(void)
{
  DEBUG_STR_LN("\n========================================");
  DEBUG_STR_LN("OTA completed successfully!");
  DEBUG_STR_LN("========================================\n");

  // Close connection
  disconnect(CLIENT_SOCKET);
  close(CLIENT_SOCKET);
  ota_active = 0;

  // Verify the written firmware one more time
  DEBUG_STR_LN("Verifying firmware in W25Q64...");
  ota_image_hdr_t verify_hdr;
  W25Q_Read(0, (uint8_t*) &verify_hdr, sizeof(ota_image_hdr_t));

  if(verify_hdr.magic == APP_MAGIC)
  {
    DEBUG_STR_LN("Firmware verification PASSED!");
    DEBUG_STR("Firmware size: ");
    DEBUG_NUM(verify_hdr.image_size);
    DEBUG_STR_LN("");
  }
  else
  {
    DEBUG_STR_LN("WARNING: Firmware verification FAILED!");
  }

  // Set bootloader flag
  DEBUG_STR_LN("Setting OTA flag for bootloader...");
  enable_ota_request();

  HAL_Delay(500);

  DEBUG_STR_LN("Resetting system...");
  HAL_Delay(200);

  NVIC_SystemReset();
}

static void ota_error(void)
{
  DEBUG_STR_LN("\n========================================");
  DEBUG_STR_LN("OTA ERROR! Update failed.");
  DEBUG_STR_LN("========================================\n");

  disconnect(CLIENT_SOCKET);
  close(CLIENT_SOCKET);
  ota_active = 0;
  ota_session_active = 0;

  // Don't set bootloader flag, just resume application
  DEBUG_STR_LN("Resuming application without OTA...");
}
