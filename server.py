#!/usr/bin/env python3
import zlib
import struct
import socket
import time
import os

# Configuration
OTA_MAGIC = 0xABCDEFAB      
OTA_VERSION = 0x02 

APP_BIN = "application.bin"      # Input application binary
OUTPUT_IMAGE = "ota_image.bin"   # Output OTA image with header

SERVER_IP = "0.0.0.0"
SERVER_PORT = 5678
CHUNK_SIZE = 512
CHUNK_DELAY = 0.25   # 250 ms

START_CMD = b"GET firmware\n"     

def create_ota_image(input_file, output_file):
    """Create OTA image by adding header to application binary"""
    print(f"Creating OTA image from {input_file}...")
    
    with open(input_file, "rb") as f:
        app = f.read()
    
    crc = zlib.crc32(app) & 0xFFFFFFFF
    size = len(app)
    
    hdr = struct.pack(
        "<IIII",
        OTA_MAGIC,          # magic
        size,
        crc,
        OTA_VERSION          # version
    )
    
    with open(output_file, "wb") as f:
        f.write(hdr)
        f.write(app)
    
    print(f"  - Size: {size} bytes")
    print(f"  - CRC32: 0x{crc:08X}")
    print(f"  - Output: {output_file}")
    return size

def serve_ota_image(image_file):
    """Serve OTA image over TCP socket"""
    if not os.path.exists(image_file):
        print(f"Error: {image_file} not found!")
        return False
    
    with open(image_file, "rb") as f:
        firmware = f.read()
    
    file_size = len(firmware)
    print(f"\nFirmware size: {file_size} bytes")
    
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((SERVER_IP, SERVER_PORT))
    srv.listen(1)
    
    print(f"OTA Server listening on port {SERVER_PORT}...")
    conn, addr = srv.accept()
    print(f"Client connected from {addr}")
    
    # Wait for START command
    print("Waiting for START command from client...")
    cmd = conn.recv(64)
    print(f"Received: {cmd.strip()}")
    print("Sending firmware...")
    
    # Send firmware in chunks
    sent = 0
    chunk_id = 0
    
    while sent < file_size:
        chunk = firmware[sent:sent + CHUNK_SIZE]
        conn.sendall(chunk)
        print(f"[TX] Chunk {chunk_id}, {len(chunk)} bytes")
        
        sent += len(chunk)
        chunk_id += 1
        
        # Pace transfer
        time.sleep(CHUNK_DELAY)
    
    print("OTA transfer finished")
    time.sleep(0.2)
    
    conn.close()
    srv.close()
    return True

def main():
    """Main function - creates OTA image and serves it"""
    print("=== OTA Image Creator & Server ===\n")
    
    # Check if input application exists
    if not os.path.exists(APP_BIN):
        print(f"Error: {APP_BIN} not found!")
        print(f"Please ensure {APP_BIN} is in the current directory.")
        return
    
    # Step 1: Create OTA image
    create_ota_image(APP_BIN, OUTPUT_IMAGE)
    
    # Step 2: Serve the image
    print("\n" + "="*40)
    serve_ota_image(OUTPUT_IMAGE)

if __name__ == "__main__":
    main()