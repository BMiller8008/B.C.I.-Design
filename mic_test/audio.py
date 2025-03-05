import socket
import numpy as np
import soundfile as sf

# Configuration
UDP_IP = "0.0.0.0"
UDP_PORT = 8080
SAMPLE_RATE = 16000  # BROKEN 
DURATION = 5  # Seconds to record
OUTPUT_WAV = "wifi_audio_fixed.wav"

# Setup UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on {UDP_IP}:{UDP_PORT}...")

samples = []
expected_samples = SAMPLE_RATE * DURATION

while len(samples) < expected_samples:
    data, addr = sock.recvfrom(8192)  # ✅ Increase buffer size to reduce drops
    print(f"Received {len(data)} bytes from {addr}")

    # ✅ Print first 10 bytes for debugging
    print(f"Raw Data (first 10 bytes): {data[:10]}")

    # ✅ Convert 16-bit ADC values correctly
    for i in range(0, len(data), 2):  # 2 bytes per sample (uint16)
        if i + 2 <= len(data):
            adc_value = int.from_bytes(data[i:i+2], "little", signed=False)
            print(f"Raw ADC: {adc_value}")  # Debug: Print first few values
            normalized_value = ((adc_value - 2048) / 2048.0)  # Normalize (-1 to 1)
            samples.append(normalized_value)

sock.close()

# Ensure we have enough samples
if len(samples) < expected_samples:
    print(f"⚠ Warning: Expected {expected_samples} samples, but received {len(samples)}.")
elif len(samples) > expected_samples:
    samples = samples[:expected_samples]  # Trim excess

# Save to WAV
sf.write(OUTPUT_WAV, np.array(samples, dtype=np.float32), SAMPLE_RATE)
print(f"✅ Audio saved to {OUTPUT_WAV} at {SAMPLE_RATE} Hz")
