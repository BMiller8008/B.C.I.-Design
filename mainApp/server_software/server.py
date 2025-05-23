import socket
import threading
import time
import queue

# === CONFIG ===
# ESP32_IP = "192.168.1.11"     # ESP32's IP
ESP32_TCP_PORT = 8081        # ESP32's TCP listener (you send to ESP32)
ESP32_CMD_RECV_PORT = 8088   # PC listens here for ESP32 → PC command messages
SERVER_UDP_PORT = 8080       # PC receives audio via UDP here



# === UDP Receiver (audio from ESP32) ===
def udp_server():
    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_sock.bind(("0.0.0.0", SERVER_UDP_PORT))
    print(f"[UDP] Listening for audio data on port {SERVER_UDP_PORT}...")

    while True:
        data, addr = udp_sock.recvfrom(1024)
        print(f"[UDP] Received {len(data)} bytes from {addr}")

# === TCP Client (PC → ESP32) to send control
# def tcp_control_sender():
#     while True:
#         try:
#             print(f"[TCP] Connecting to ESP32 at {ESP32_IP}:{ESP32_TCP_PORT}...")
#             sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#             sock.settimeout(5)  # Short timeout for connect attempts
#             sock.connect((ESP32_IP, ESP32_TCP_PORT))
#             sock.settimeout(None)  # Restore blocking mode
#             print("[TCP] Connected!")

#             while True:
#                 msg = input("[TCP] Enter control message: ")
#                 if msg.strip() == "":
#                     continue
#                 try:
#                     sock.sendall(msg.encode() + b"\n")
#                 except BrokenPipeError:
#                     print("[TCP] Disconnected. Will reconnect...")
#                     break

#         except (socket.timeout, ConnectionRefusedError, OSError) as e:
#             print(f"[TCP] Could not connect: {e}. Retrying in 2s...")
#             time.sleep(2)
#         except Exception as e:
#             print(f"[TCP] Unexpected error: {e}")
#             time.sleep(2)
#         finally:
#             try:
#                 sock.close()
#             except:
#                 pass

# === automated translated text for TCP Client (PC → ESP32) to send control messages ===

def tcp_control_sender(text_queue, config):

    while True:
        esp32_ip = config.get_ip()
        if not esp32_ip:
            print("[TCP] Waiting for ESP32 IP to be detected via UDP...")
            time.sleep(1)
            continue
        try:
            print(f"[TCP] Connecting to ESP32 at {esp32_ip}:{ESP32_TCP_PORT}...")
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((esp32_ip, ESP32_TCP_PORT))
            print("[TCP] Connected!")

            while True:
                # Wait for a new text chunk
                control_msg = text_queue.get()
                try:
                    sock.sendall(control_msg.encode())
                    print(f"[TCP] Sent: {control_msg}")
                    # wait 0.5 seconds before sending the next message
                    time.sleep(1)
                except BrokenPipeError:
                    print("[TCP] Disconnected. Will reconnect...")
                    break

        except Exception as e:
            print(f"[TCP] Connection error: {e}")
            time.sleep(2)


# === TCP Server (PC ← ESP32) to receive status/commands
def tcp_command_receiver(text_queue,config):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_sock:
        server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_sock.bind(("0.0.0.0", ESP32_CMD_RECV_PORT))
        server_sock.listen(5)
        print(f"[CMD] Listening for ESP32 commands on port {ESP32_CMD_RECV_PORT}...")

        while True:
            conn, addr = server_sock.accept()
            with conn:
                print(f"[CMD] ESP32 connected from {addr}")
                data = conn.recv(1024)
                if data:
                    message = data.decode().strip()
                    print(f"[CMD] Received from ESP32: {data.decode().strip()}")
                    updates = {}
                    for pair in message.split(","):
                        if ":" in pair:
                            key, val = pair.split(":", 1)
                            updates[key.strip()] = val.strip()
                    config.update(updates)
                    #clear the text queue buffer
                    try:
                        while True:
                            text_queue.get_nowait()
                    except queue.Empty:
                        pass

                else:
                    print("[CMD] ESP32 closed the connection.")

# === Start All Threads ===
if __name__ == "__main__":
    threading.Thread(target=udp_server, daemon=True).start()
    threading.Thread(target=tcp_command_receiver, daemon=True).start()
    tcp_control_sender()
