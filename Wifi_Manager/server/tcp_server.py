import socket

ESP32_IP = "192.168.1.4"  # Replace with your ESP32's IP
ESP32_PORT = 8081

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    print("Connecting...")
    sock.connect((ESP32_IP, ESP32_PORT))
    print("Connected!")
    sock.sendall(b"hello esp\n")
    print("Message sent.")
