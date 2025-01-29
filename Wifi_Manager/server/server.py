import socket

# Server Configuration
HOST = "0.0.0.0"  # Listen on all available network interfaces
PORT = 8080        # Must match the ESP32 sender port

# Create a UDP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((HOST, PORT))

print(f"Listening for UDP packets on {HOST}:{PORT}...")

while True:
    data, addr = server_socket.recvfrom(1024)  # Receive up to 1024 bytes
    print(f"Received {len(data)} bytes from {addr}: {data}")
