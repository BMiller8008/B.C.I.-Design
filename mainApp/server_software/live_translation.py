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
import string

import noisereduce as nr
import webrtcvad

# Load keys from .env
load_dotenv()
AZURE_SPEECH_KEY       = os.getenv("AZURE_SPEECH_KEY")
AZURE_SPEECH_REGION    = os.getenv("AZURE_SPEECH_REGION")
AZURE_TRANSLATOR_KEY   = os.getenv("AZURE_TRANSLATOR_KEY")
AZURE_TRANSLATOR_REGION= os.getenv("AZURE_TRANSLATOR_REGION")

SPEECH_API_URL      = (
    f"https://{AZURE_SPEECH_REGION}.stt.speech.microsoft.com"
    "/speech/recognition/conversation/cognitiveservices/v1"
)
TRANSLATION_API_URL = (
    "https://api.cognitive.microsofttranslator.com/translate?api-version=3.0"
)

# Constants
UDP_PORT        = 8080
SAMPLE_RATE     = 16000
WINDOW_SECS     = 5.0
OVERLAP_SECS    = 0.5
WINDOW_SAMPLES  = int(SAMPLE_RATE * WINDOW_SECS)
OVERLAP_SAMPLES = int(SAMPLE_RATE * OVERLAP_SECS)
LOG_FILE        = "translation_log.txt"

# 1) Band-pass filter setup (300–3400 Hz Butterworth)
lowcut    = 300.0
highcut   = 3400.0
low_norm  = lowcut  / (SAMPLE_RATE / 2.0)
high_norm = highcut / (SAMPLE_RATE / 2.0)
b, a      = iirfilter(
    N=6,
    Wn=[low_norm, high_norm],
    btype='band',
    ftype='butter'
)
zi = lfilter_zi(b, a)

# 2) Voice activity detector (WebRTC VAD)
vad = webrtcvad.Vad(2)  # aggressiveness 0–3

def log_to_file(text: str):
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(text + "\n")

def send_to_azure(audio: np.ndarray, text_queue: Queue, chunk_size: int, language_code: str):
    # Write to temp file
    sf.write("temp.wav", audio, SAMPLE_RATE)
    with open("temp.wav", "rb") as f:
        wav_data = f.read()

    # Transcription
    headers = {
        "Ocp-Apim-Subscription-Key": AZURE_SPEECH_KEY,
        "Content-Type": "audio/wav"
    }
    params = {"language": language_code, "format": "simple"}
    t0 = time.time()
    resp = requests.post(SPEECH_API_URL, headers=headers, params=params, data=wav_data)
    resp.raise_for_status()
    speech_time = round((time.time() - t0) * 1000, 2)

    text = resp.json().get("DisplayText", "").strip()
    if not text:
        print("⚠️ No transcription received.")
        return
    print(f"🗣️ Transcribed: {text} ({speech_time}ms)")

    # Translation
    headers = {
        "Ocp-Apim-Subscription-Key": AZURE_TRANSLATOR_KEY,
        "Ocp-Apim-Subscription-Region": AZURE_TRANSLATOR_REGION,
        "Content-Type": "application/json"
    }
    body = [{"Text": text}]
    t1 = time.time()
    resp2 = requests.post(
        TRANSLATION_API_URL,
        headers=headers,
        json=body,
        params={"to": "en"}
    )
    resp2.raise_for_status()
    translation_time = round((time.time() - t1) * 1000, 2)

    translated = resp2.json()[0]["translations"][0]["text"]
    print(f"🌐 Translated: {translated} ({translation_time}ms)")

    timestamp = datetime.now().strftime("[%H:%M:%S]")
    log_to_file(f"{timestamp} 🗣 {text} → 🌐 {translated}")

    # Chunk & pad for display
    max_char = {4: 21, 3: 15, 2: 8}.get(chunk_size, 15)
    words = translated.strip().split()
    for i in range(0, len(words), chunk_size):
          chunk = " ".join(words[i:i+chunk_size])
          chunk_and_pad_text(chunk, max_char, text_queue)

def chunk_and_pad_text(translated_text: str, max_chars: int, text_queue: Queue):
    words = [w.strip(string.punctuation) for w in translated_text.split()]
    lines = []
    chunk = ""
    current_len = 0

    for word in words:
        wl   = len(word)
        need = wl + (1 if chunk else 0)

        # Case A: single word longer than line → hyphenate
        if current_len == 0 and wl > max_chars:
            rem = word
            while len(rem) > max_chars:
                lines.append(rem[:max_chars-1] + "-")
                rem = rem[max_chars-1:]
            chunk = rem
            current_len = len(rem)
            continue

        # Case B: adding would overflow → split mid-word if needed
        if current_len + need > max_chars:
            fit = max_chars - current_len - 1  # reserve 1 for hyphen
            if fit > 0:
                piece     = word[:fit]
                remainder = word[fit:]
                line = (chunk + " " if chunk else "") + piece + "-"
                lines.append(line)
                chunk = remainder
                current_len = len(remainder)
            else:
                lines.append(chunk + " " * (max_chars - current_len))
                chunk = word
                current_len = wl

        else:
            # Normal append
            if chunk:
                chunk += " "
                current_len += 1
            chunk += word
            current_len += wl

    # Final line (pad, no dash)
    if chunk:
        lines.append(chunk + " " * (max_chars - current_len))

    text_queue.put(" ".join(lines))

def process_audio_from_udp(text_queue: Queue, config):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", UDP_PORT))
    print(f"🔊 Listening on UDP port {UDP_PORT}...")

    audio_buf    = []
    filter_state = zi * 0.0
    got_ip       = False

    while True:
        start, chunk_size, lang = config.snapshot()
        if not start:
            time.sleep(0.1)
            continue

        data, addr = sock.recvfrom(4096)

        # Capture IP once
        if not got_ip:
            config.set_ip(addr[0])
            got_ip = True
            print(f"📡 Detected ESP32 IP: {addr[0]}")

        # Decode PCM little-endian unsigned 12-bit → float32
        pcm     = np.frombuffer(data, dtype=np.uint16).astype(np.float32)
        samples = (pcm - 2048.0) / 2048.0

        # 1) Band-pass filter
        filtered, filter_state = lfilter(b, a, samples, zi=filter_state)
        audio_buf.extend(filtered.tolist())

        # 2) When enough collected, process chunk
        if len(audio_buf) >= WINDOW_SAMPLES:
            seg = np.array(audio_buf[:WINDOW_SAMPLES], dtype=np.float32)
            audio_buf = audio_buf[int(WINDOW_SAMPLES*(1-OVERLAP_SECS/WINDOW_SECS)):]

            # 3) Spectral noise reduction
            denoised = nr.reduce_noise(y=seg, sr=SAMPLE_RATE)

            # 4) VAD: skip if no speech
            pcm_bytes = (denoised * 32767).astype(np.int16).tobytes()
            frame_n   = int(0.03 * SAMPLE_RATE) * 2
            if not any(
                vad.is_speech(pcm_bytes[i:i+frame_n], SAMPLE_RATE)
                for i in range(0, len(pcm_bytes), frame_n)
            ):
                print("[VAD] Silence detected; skipping segment.")
                continue

            # 5) Send cleaned speech to Azure
            send_to_azure(denoised, text_queue, chunk_size, lang)
