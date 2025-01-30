import socket
import time

# Server Configuration
HOST = "0.0.0.0"
PORT = 8080

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((HOST, PORT))

print(f"Listening for UDP packets on {HOST}:{PORT}...\n")

while True:
    data, addr = server_socket.recvfrom(1024)
    print(f"Received {len(data)} bytes from {addr}: {data}")

    # Send response **to ESP32's receive port (8081)**
    response = "Hello from server!"
    server_socket.sendto(response.encode(), (addr[0], 8081))  # ✅ Send to ESP32 receive port

    time.sleep(1)  # Prevent spamming
