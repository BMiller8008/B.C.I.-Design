import serial
import wave
import numpy as np
import time

SERIAL_PORT = 'COM10'  # Change to your ESP32 port
BAUD_RATE = 115200
SAMPLE_RATE = 16000  # 16 kHz
DURATION_SECONDS = 5  # Record for 5 seconds
TOTAL_SAMPLES = SAMPLE_RATE * DURATION_SECONDS  # 80,000 samples for 5 sec
OUTPUT_FILENAME = "test.wav"

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
audio_data = []

print(f"🎤 Recording {TOTAL_SAMPLES} samples from ESP32...")

start_time = time.time()  # Track start time

try:
    while len(audio_data) < TOTAL_SAMPLES:
        if ser.in_waiting:
            try:
                sample = int(ser.readline().decode('utf-8').strip())
                audio_data.append(sample)
            except ValueError:
                pass  # Ignore corrupt lines

        # ⏳ Stop after 5 seconds, even if samples are missing
        if time.time() - start_time > DURATION_SECONDS:
            print("⏳ Timeout reached, stopping recording.")
            break

except KeyboardInterrupt:
    print("Recording stopped by user")

ser.close()

# Convert to numpy array and normalize
if len(audio_data) == 0:
    print("❌ No samples recorded. Check serial connection.")
    exit()

# Normalize PCM to 16-bit integer range
audio_np = np.array(audio_data, dtype=np.int16)

print(f"📊 Total valid samples recorded: {len(audio_np)}")
print(f"🔍 First 10 samples: {audio_np[:10]}")

# Save as WAV file
with wave.open(OUTPUT_FILENAME, "w") as wav_file:
    wav_file.setnchannels(1)  # Mono
    wav_file.setsampwidth(2)  # 16-bit PCM
    wav_file.setframerate(SAMPLE_RATE)
    wav_file.writeframes(audio_np.tobytes())  # Ensure full frames are written

print(f"✅ Successfully saved {len(audio_np)} samples to {OUTPUT_FILENAME}")

# Verify final file size
import os
print(f"📂 File size: {os.path.getsize(OUTPUT_FILENAME)} bytes")
