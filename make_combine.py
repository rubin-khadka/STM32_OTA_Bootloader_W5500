import struct, zlib, sys, os

APP_BIN = "STM32_Application_v1.0.bin"
OUT_BIN = "combined.bin"

HEADER_ADDR = 0x08004000    # APP_HEADER_ADDR
HEADER_SIZE = 1024          # 1 KB page
MAGIC       = 0xABCDEFAB    # Magic
VERSION     = 1             # Version

if not os.path.exists(APP_BIN):
    print(f"ERROR: {APP_BIN} not found"); sys.exit(1)

app_data = open(APP_BIN, "rb").read()
size = len(app_data)
crc  = zlib.crc32(app_data) & 0xFFFFFFFF

if size == 0 or size > 47 * 1024:
    print(f"ERROR: app size {size} invalid (max 47 KB)"); sys.exit(1)

print(f"App file: {APP_BIN}")
print(f"App size: {size} bytes (0x{size:X})")
print(f"CRC-32:   0x{crc:08X}")

# Build header — 5 words (20 bytes)
header = struct.pack("<IIIII", 0, MAGIC, size, crc, VERSION)
header += b"\xFF" * (HEADER_SIZE - len(header))

# Combined = header page + app bytes
combined = header + app_data
open(OUT_BIN, "wb").write(combined)

print(f"\n Generated: {OUT_BIN}")
print(f"   Total size: {len(combined)} bytes")
print(f"   Flash at:   0x{HEADER_ADDR:08X}")