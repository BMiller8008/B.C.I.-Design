import socket
import numpy as np
import soundfile as sf
from scipy.signal import iirfilter, lfilter, lfilter_zi
from dotenv import load_dotenv
import requests
import time
import os
from datetime import datetime
from queue import Queue

# Load keys from .env
load_dotenv()
AZURE_SPEECH_KEY = os.getenv("AZURE_SPEECH_KEY")
AZURE_SPEECH_REGION = os.getenv("AZURE_SPEECH_REGION")
AZURE_TRANSLATOR_KEY = os.getenv("AZURE_TRANSLATOR_KEY")
AZURE_TRANSLATOR_REGION = os.getenv("AZURE_TRANSLATOR_REGION")

SPEECH_API_URL = f"https://{AZURE_SPEECH_REGION}.stt.speech.microsoft.com/speech/recognition/conversation/cognitiveservices/v1"
TRANSLATION_API_URL = "https://api.cognitive.microsofttranslator.com/translate?api-version=3.0"

# Constants
UDP_PORT = 8080
SAMPLE_RATE = 16000
CHUNK_SIZE = 1024
WINDOW_SECS = 5.0
OVERLAP_SECS = 0.5
WINDOW_SAMPLES = int(SAMPLE_RATE * WINDOW_SECS)
OVERLAP_SAMPLES = int(SAMPLE_RATE * OVERLAP_SECS)
TARGET_LANGUAGE = "en"
LOG_FILE = "translation_log.txt"

# Bandpass filter setup
lowcut = 300.0
highcut = 3400.0
low_norm = lowcut / (SAMPLE_RATE / 2.0)
high_norm = highcut / (SAMPLE_RATE / 2.0)
b, a = iirfilter(N=6, Wn=[low_norm, high_norm], btype='band', ftype='butter')
zi = lfilter_zi(b, a)

def log_to_file(text):
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(text + "\n")

def send_to_azure(audio_data, text_queue, chunk_size, language_code):
    # Write to temp file
    sf.write("temp.wav", audio_data, SAMPLE_RATE)

    with open("temp.wav", "rb") as f:
        wav_data = f.read()

    # --- Transcription ---
    headers = {
        "Ocp-Apim-Subscription-Key": AZURE_SPEECH_KEY,
        "Content-Type": "audio/wav",
        "Accept": "application/json"
    }
    params = {
        "language": language_code,
        "format": "simple"
    }
    speech_start = time.time()
    response = requests.post(SPEECH_API_URL, headers=headers, params=params, data=wav_data)
    speech_time = round((time.time() - speech_start) * 1000, 2)

    if response.status_code != 200:
        print("❌ Speech API error:", response.text)
        return

    speech_data = response.json()
    text = speech_data.get("DisplayText", "").strip()
    if not text:
        print("⚠️ No transcription received.")
        return

    print(f"🗣️  Transcribed: {text} ({speech_time}ms)")

    # --- Translation ---
    translation_headers = {
        "Ocp-Apim-Subscription-Key": AZURE_TRANSLATOR_KEY,
        "Ocp-Apim-Subscription-Region": AZURE_TRANSLATOR_REGION,
        "Content-Type": "application/json"
    }
    translation_body = [{"Text": text}]
    translation_start = time.time()
    translation_response = requests.post(
        TRANSLATION_API_URL,
        headers=translation_headers,
        json=translation_body,
        params={"to": TARGET_LANGUAGE}
    )
    translation_time = round((time.time() - translation_start) * 1000, 2)

    if translation_response.status_code != 200:
        print("❌ Translation API error:", translation_response.text)
        return

    translation_data = translation_response.json()
    translated_text = translation_data[0]["translations"][0]["text"]
    print(f"🌐 Translated: {translated_text} ({translation_time}ms)")

    timestamp = datetime.now().strftime("[%H:%M:%S]")
    log_to_file(f"{timestamp} 🗣 {text} → 🌐 {translated_text}")

    # Split into chunks of 3 words and add to queue
    words = translated_text.strip().split()
    for i in range(0, len(words), chunk_size):
            chunk = " ".join(words[i:i+chunk_size])
            text_queue.put(chunk)

def process_audio_from_udp(text_queue: Queue, config):
    # Setup socket and filter state
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", UDP_PORT))
    print(f"🔊 Listening on UDP port {UDP_PORT}...")

    audio_buffer = []
    filter_state = zi * 0.0
    esp32_ip_captured = False

    while True:

        start_flag, chunk_size, language_code = config.snapshot()
        # print(f"🔄 Config: start={start_flag}, chunk_size={chunk_size}, language_code={language_code}")
        if not start_flag:
            time.sleep(0.1)
            continue

        data, addr = sock.recvfrom(4096)

         # Capture ESP32 IP once
        if not esp32_ip_captured:
            config.set_ip(addr[0])
            esp32_ip_captured = True
            print(f"📡 Detected ESP32 IP from UDP: {addr[0]}")
        

        chunk = []
        for i in range(0, len(data), 2):
            if i + 1 < len(data):
                val = int.from_bytes(data[i:i+2], "little", signed=False)
                norm = (val - 2048) / 2048.0
                chunk.append(norm)

        if not chunk:
            continue

        chunk = np.array(chunk, dtype=np.float32)
        filtered, filter_state = lfilter(b, a, chunk, zi=filter_state)
        audio_buffer.extend(filtered.tolist())

        if len(audio_buffer) >= WINDOW_SAMPLES:
            segment = np.array(audio_buffer[:WINDOW_SAMPLES], dtype=np.float32)
            send_to_azure(segment, text_queue, chunk_size, language_code)
            audio_buffer = audio_buffer[int(WINDOW_SAMPLES - OVERLAP_SAMPLES):]
