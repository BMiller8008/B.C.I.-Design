import socket

# Listen on all interfaces
HOST = ""  # ✅ Allows receiving from any interface (instead of "0.0.0.0")
PORT = 8080

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((HOST, PORT))

print(f"Listening for UDP packets on port {PORT}...")

while True:
    data, addr = server_socket.recvfrom(1024)  # Receive up to 1024 bytes
    print(f"Received {len(data)} bytes from {addr}: {data}")
