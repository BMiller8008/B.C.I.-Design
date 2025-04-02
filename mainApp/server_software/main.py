import threading
from queue import Queue
from server import tcp_control_sender, tcp_command_receiver
from live_translation import process_audio_from_udp

# Create a shared queue for passing translated text chunks
text_queue = Queue()

# Thread wrapper for the audio + translation thread
def audio_translation_thread():
    print("[Thread] Audio + Translation thread started.")
    process_audio_from_udp(text_queue)

# Thread wrapper for the TCP sender (now non-blocking)
def tcp_control_sender_thread():
    print("[Thread] TCP Control Sender started.")
    tcp_control_sender(text_queue)

if __name__ == "__main__":
    # Thread 1: Audio processing + transcription + translation
    t1 = threading.Thread(target=audio_translation_thread, daemon=True)

    # Thread 2: TCP server to receive commands from ESP32
    t2 = threading.Thread(target=tcp_command_receiver, daemon=True)

    # Thread 3: TCP client to send translated text to ESP32
    t3 = threading.Thread(target=tcp_control_sender_thread, daemon=True)

    # Start all threads
    t1.start()
    t2.start()
    t3.start()

    # Keep the main thread alive
    t1.join()
    t2.join()
    t3.join()
