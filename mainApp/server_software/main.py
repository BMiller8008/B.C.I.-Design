import threading
from queue import Queue
from config import RuntimeConfig
from server import tcp_control_sender, tcp_command_receiver
from live_translation import process_audio_from_udp

# Create a shared queue for passing translated text chunks
config = RuntimeConfig()
text_queue = Queue()

# Thread wrapper for the audio + translation thread
def audio_translation_thread():
    print("[Thread] Audio + Translation thread started.")
    process_audio_from_udp(text_queue, config)

# Thread wrapper for the TCP sender (now non-blocking)
def tcp_control_sender_thread():
    print("[Thread] TCP Control Sender started.")
    tcp_control_sender(text_queue, config)

# Thread wrapper for the TCP command receiver (now non-blocking)
def tcp_command_receiver_thread():
    print("[Thread] Command Receiver started.")
    tcp_command_receiver(text_queue, config)

if __name__ == "__main__":
    # Thread 1: Audio processing + transcription + translation
    t1 = threading.Thread(target=audio_translation_thread, daemon=True)

    # Thread 2: TCP server to receive commands from ESP32
    t2 = threading.Thread(target=tcp_command_receiver, args=(text_queue, config,), daemon=True)

    # commands we need, "start",  size_array, language_array
    # start: while 1 the process is running, 0: stop the process
    # size_array: size of the array to be used for the translation [0: 8, 1:12, 2:16]
    # language_array: language to be used for the translation [0: english, 1: spanish, 2: Hindi, 3: Mandarin, 4: Arabic, 5:Portuguese] 

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
