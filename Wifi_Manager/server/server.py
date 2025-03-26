import socket
import threading
import time

# === CONFIG ===
ESP32_IP = "192.168.1.4"     # ESP32's IP
ESP32_TCP_PORT = 8081        # ESP32's TCP listener port
SERVER_UDP_PORT = 8080       # PC's UDP port for incoming audio

# === UDP Receiver (audio from ESP32) ===
def udp_server():
    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_sock.bind(("0.0.0.0", SERVER_UDP_PORT))
    print(f"[UDP] Listening for audio data on port {SERVER_UDP_PORT}...")

    while True:
        data, addr = udp_sock.recvfrom(1024)
        print(f"[UDP] Received {len(data)} bytes from {addr}")

# === TCP Client (control to ESP32) ===
def tcp_control_sender():
    print(f"[TCP] Connecting to ESP32 at {ESP32_IP}:{ESP32_TCP_PORT}...")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((ESP32_IP, ESP32_TCP_PORT))
        print("[TCP] Connected!")

        # Send control messages periodically
        while True:
            msg = input("[TCP] Enter control message: ")
            if msg.strip() == "":
                continue
            sock.sendall(msg.encode() + b"\n")

# === Start Both Threads ===
if __name__ == "__main__":
    threading.Thread(target=udp_server, daemon=True).start()
    tcp_control_sender()
