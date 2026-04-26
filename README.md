# STM32 OTA Bootloader with W5500 Ethernet

[![STM32](https://img.shields.io/badge/STM32-F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![CubeIDE](https://img.shields.io/badge/IDE-STM32CubeIDE-darkblue)](http://st.com/en/development-tools/stm32cubeide.html)
[![CRC32](https://img.shields.io/badge/CRC-32-darkgreen)](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)

## Project Overview

This project implements a Over-The-Air (OTA) firmware update system for STM32F103C8T6 microcontrollers. The system allows firmware updates to be sent from a PC server to the embedded device over Ethernet using the W5500 controller.

**Bootloader** – Runs on every power-up:
- Checks if an OTA update is pending (reads a flag from flash)
- If no update pending: calculates CRC of existing application, verifies magic number, and jumps to it
- If update pending: reads new firmware from external W25Q64 flash, verifies its CRC and magic number, writes it to internal flash, then resets

**Main Application** – Normal device operation:
- Runs sensors and LCD display
- When button is pressed: connects to PC server via Ethernet, downloads firmware, stores it in W25Q64 external flash, sets OTA flag, and resets

**PC Server** – Python script that:
- Listens on port 5678
- Sends complete firmware image (16-byte header + binary) when requested

The application is a simplified version of the [Multi Sensor Cloud Logger Project](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger). It uses three sensors:

DHT11 - Temperature & Humidity
MPU6050 - Accelerometer & Gyroscope
DS3231 - Real Time Clock
A 16x2 LCD display shows sensor values, and a push button cycles through different display modes.


## Video Demonstrations

### Hardware Demo

https://github.com/user-attachments/assets/7f80a6b2-bdb8-48e4-85a1-f0f5051a6c1d

When powered on, the bootloader checks everything and runs Application v1. Application v1 uses two sensors: DHT11 and DS3231. When the button is pressed, OTA update starts (displayed on LCD). After downloading and installing the new firmware, Application v2 loads, which now uses three sensors: DHT11, DS3231, and MPU6050.

### PC Server

https://github.com/user-attachments/assets/19c6f0b3-9bce-46eb-b01b-5a8af232cd05

Shows Hercules UART debug and PC server running, waiting for confirmation from the STM32 client. When the button is pressed on the client, the server sends data in chunks of 512 bytes.

## Project Schematic

<img width="1512" height="752" alt="Schematic Diagram" src="https://github.com/user-attachments/assets/a18a1107-f437-4b55-bfec-7237480d4d15" />


## Pin Configuration

| Component | STM32 Pin | Notes |
|-----------|-----------|-------|
| Button (OTA Trigger) | PA1 | Starts OTA update |
| Button (Mode Switch) | PA0 | Cycles LCD display modes |
| DHT11 | PB0 | Single-wire data |
| LCD (I2C) | PB10 (SCL), PB11 (SDA) | With PCF8574 backpack |
| MPU6050 | PB10 (SCL), PB11 (SDA) | I2C address: 0x69 |
| DS3231 | PB10 (SCL), PB11 (SDA) | I2C address: 0x68 |
| W5500 (Ethernet) | PA5 (SCK), PA6 (MISO), PA7 (MOSI), PA4 (CS), PA3 (RESET) | SPI1 |
| W25Q64 (External Flash) | PB13 (SCK), PB14 (MISO), PB15 (MOSI), PB12 (CS) | SPI2 |
| USART1 (Debug) | PA9 (TX), PA10 (RX) | 115200 baud, 8N1 |

> **Note:** Multiple I2C devices (LCD, MPU6050, DS3231) share the same I2C bus (PB10/PB11) with different addresses.