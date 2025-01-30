import socket
import struct
import time

# Server Configuration
HOST = "0.0.0.0"  # Listen on all available network interfaces
PORT = 8080        # Must match the ESP32 sender port

# Create a UDP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((HOST, PORT))

print(f"Listening for UDP packets on {HOST}:{PORT}...\n")

while True:
    data, addr = server_socket.recvfrom(1024)  # Receive up to 1024 bytes
    timestamp = time.strftime("%H:%M:%S", time.localtime())  # Get current time


    if not data:
        continue

    # Calculate the number of integers in the received data
    num_ints = len(data) // 4  # Each int is 4 bytes
    unpacked_data = struct.unpack(f"{num_ints}i", data)  # Convert bytes to ints

    print(f"[{timestamp}] Received {len(data)} bytes from {addr}")
    print("   Raw data: " + str(data))
    print("   Parsed Integers:", unpacked_data)
    print("-" * 50)
