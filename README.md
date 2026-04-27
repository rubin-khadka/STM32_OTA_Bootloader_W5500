# STM32 OTA Bootloader with W5500 Ethernet

[![STM32](https://img.shields.io/badge/STM32-F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![CubeIDE](https://img.shields.io/badge/IDE-STM32CubeIDE-darkblue)](http://st.com/en/development-tools/stm32cubeide.html)
[![CRC32](https://img.shields.io/badge/CRC-32-darkgreen)](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)

## Project Overview

This project implements an Over-The-Air (OTA) firmware update system for STM32F103C8T6 microcontrollers. The system allows firmware updates to be sent from a PC server to the embedded device over Ethernet using the W5500 controller.

**Bootloader** – Runs on every power-up:
- Checks if an OTA update is pending (reads a flag from flash)
- If no update pending: calculates CRC of existing application, verifies magic number and CRC, and jumps to it
- If update pending: reads new firmware from external W25Q64 flash, verifies its CRC and magic number, writes it to internal flash, clears OTA flag, then jumps to the new application

**Main Application** – Normal device operation:
- Runs sensors and LCD display
- When button is pressed: connects to PC server via Ethernet, downloads firmware, stores it in W25Q64 external flash, sets OTA flag, and resets

**PC Server** – Python script that:
- Listens on port 5678
- Sends complete firmware image (16-byte header + binary) when requested

The application is a simplified version of the [Multi Sensor Cloud Logger Project](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger). It uses three sensors:

- DHT11 - Temperature & Humidity
- MPU6050 - Accelerometer & Gyroscope (scaled)
- DS3231 - Real Time Clock
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

## Memory Layout

The STM32 flash memory is partitioned to separate the bootloader from the application firmware:

| Start Address | Size | Section | Description |
|---------------|------|---------|-------------|
| 0x08000000 | 16KB | Bootloader | Custom bootloader code |
| 0x08004400 | 47KB | Application | Main application firmware |

```
Flash Memory Map (STM32F103C8 - 64KB total)
┌─────────────────────────────────────────────────────────────┐
│ 0x08000000                                                  │
│    ┌─────────────────────────────────────────────────────┐  │
│    │  Bootloader                     (16KB)              │  │
│    │  0x08000000 - 0x08003FFF                            │  │
│    └─────────────────────────────────────────────────────┘  │
│ 0x08004400                                                  │
│    ┌─────────────────────────────────────────────────────┐  │
│    │  Application Firmware           (47KB)              │  │
│    │  0x08004400 - 0x0800FFFF                            │  │
│    └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```
- Bootloader starts at 0x08000000 (default vector table address)
- Application code starts at 0x08004400 with custom vector table remapping
- Bootloader verifies CRC from header before jumping to the application

### External Flash Memory

The external W25Q64 flash stores incoming firmware during OTA update:
- Firmware is written starting at address 0x00000000
- Bootloader reads from this address when OTA flag is set
- Provides 8MB of storage (more than enough for multiple firmware images)

## OTA Image Format

The firmware is sent from PC to STM32 as a single binary file with the following structure:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 bytes | Magic Number | 0xABCDEFAB (identifies valid firmware) |
| 4 | 4 bytes | Image Size | Size of firmware binary in bytes |
| 8 | 4 bytes | CRC32 | Checksum of firmware binary |
| 12 | 4 bytes | Version | Firmware version number |
| 16 | variable | Firmware Binary | Actual application code |

The bootloader uses the header information to:
- **Magic Number** – Confirm the image is a valid firmware
- **CRC32** – Verify data integrity before installation
- **Image Size** – Prevent overflow beyond allocated flash space
- **Version** – Track firmware versions

## PC Server Script

The Python script performs two main tasks:

### 1. Creating the OTA Image

The script reads the raw application binary (`application.bin`) and adds a 16-byte header containing:
- **Magic Number** (0xABCDEFAB) – Identifies valid firmware
- **Image Size** – Length of the firmware in bytes
- **CRC32** – Checksum calculated from the firmware
- **Version** – Firmware version number

The output is saved as `ota_image.bin`.

### 2. Serving the Firmware

The script starts a TCP server that:
- Listens on port 5678 for incoming connections
- Waits for the "GET firmware" command from the STM32 client
- Sends the complete OTA image in 512-byte chunks
- Displays progress of each chunk sent

## CRC Calculation 

The bootloader uses a 32-bit CRC (Cyclic Redundancy Check) to ensure the integrity of the application firmware before executing it.

The CRC algorithm processes the application firmware byte by byte, performing a series of bitwise operations to generate a unique 32-bit checksum. The algorithm works as follows:

1. **Initialization** - The CRC value starts at `0xFFFFFFFF`
2. **Byte Processing** - Each byte of the application firmware is XORed with the current CRC value
3. **Bit Shifting** - For each bit in the byte (8 bits total), the algorithm checks the least significant bit:
    - If the bit is 1 → The CRC is shifted right and XORed with a fixed polynomial (`0xEDB88320`)
    - If the bit is 0 → The CRC is simply shifted right
4. **Finalization** - After all bytes are processed, the result is XORed with `0xFFFFFFFF` to produce the final CRC32 value

This polynomial (`0xEDB88320`) is the standard CRC-32 used in protocols like Ethernet, PNG, and ZIP files.

## OTA Update Flowchart

<img width="1081" height="1104" alt="flow chart" src="https://github.com/user-attachments/assets/6cf2fa4c-92f4-4e1e-a4d5-d2d6c586ce5b" />

## Related Projects 
- [STM32_MicroSD_Cloud_Logger](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger)
- [STM32_Custom_Bootloader_CRC](https://github.com/rubin-khadka/STM32_Custom_Bootloader_CRC)
- [STM32_OTA_Bootloader_ESP8266](https://github.com/rubin-khadka/STM32_OTA_Bootloader_ESP8266)

## Resources
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [DHT11 Sensor Datasheet](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)
- [MPU6050 Sensor Datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
- [RTC DS3231 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf)
- [PCF8574 I2C Backpack Datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [WIZNET W5500 Datasheet](https://cdn.sparkfun.com/datasheets/Dev/Arduino/Shields/W5500_datasheet_v1.0.2_1.pdf)
- [Winbond W25Q64 Flash Memory Datasheet](https://docs.rs-online.com/9bfc/0900766b81704060.pdf)

## Project Status
- **Status**: Complete
- **Version**: v1.0
- **Last Updated**: April 2026

## Contact
**Rubin Khadka Chhetri**  
📧 rubinkhadka84@gmail.com <br>
🐙 GitHub: https://github.com/rubin-khadka

