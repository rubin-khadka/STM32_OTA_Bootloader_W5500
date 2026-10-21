import socket
import time

SERVER_IP = "0.0.0.0"
SERVER_PORT = 5678
IMAGE_FILE = "ota_image.bin"

CHUNK_SIZE = 512
CHUNK_DELAY = 0.25   # 250 ms

START_CMD = b"START\n"   # define start command


def main():
    with open(IMAGE_FILE, "rb") as f:
        firmware = f.read()

    file_size = len(firmware)
    print(f"Firmware size: {file_size} bytes")

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((SERVER_IP, SERVER_PORT))
    srv.listen(1)

    print(f"OTA Server listening on port {SERVER_PORT}...")
    conn, addr = srv.accept()
    print(f"Client connected from {addr}")

    # ---- WAIT FOR START ----
    print("Waiting for START command from client...")
    cmd = conn.recv(64)
    print("START received. Sending firmware...")
    
#    time.sleep(1)

    # ---- SEND FIRMWARE ----
    sent = 0
    chunk_id = 0

    while sent < file_size:
        chunk = firmware[sent:sent + CHUNK_SIZE]
        conn.sendall(chunk)
        print(f"[TX] Chunk {chunk_id}, {len(chunk)} bytes")

        sent += len(chunk)
        chunk_id += 1

        # pace transfer
        time.sleep(CHUNK_DELAY)

    print("OTA transfer finished")
    time.sleep(0.2)

    conn.close()
    srv.close()


if __name__ == "__main__":
    main()
