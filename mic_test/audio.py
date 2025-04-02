import socket
import numpy as np
import soundfile as sf
from scipy.signal import iirfilter, lfilter, lfilter_zi

# -------------------------------------------------------------------
# Configuration
# -------------------------------------------------------------------
UDP_IP      = "0.0.0.0"
UDP_PORT    = 8080
SAMPLE_RATE = 16000    # match your ESP32 sample rate
RECORD_SECS = 10
OUTPUT_WAV  = "ten.wav"

# Band-pass range for voice
lowcut  = 300.0
highcut = 3400.0

# 6th-order Butterworth band-pass
low_norm  = lowcut  / (SAMPLE_RATE / 2.0)
high_norm = highcut / (SAMPLE_RATE / 2.0)
b, a = iirfilter(
    N=6,
    Wn=[low_norm, high_norm],
    btype='band',
    analog=False,
    ftype='butter'
)

# Initialize filter state
zi = lfilter_zi(b, a)
filter_state = zi * 0.0

# -------------------------------------------------------------------
# Hysteresis Gate + Gain Settings
# -------------------------------------------------------------------
# Gate has two thresholds:
# - Gate remains closed below GATE_CLOSE_THRESHOLD
# - Gate opens once RMS > GATE_OPEN_THRESHOLD
# - Typical values: GATE_CLOSE_THRESHOLD < GATE_OPEN_THRESHOLD
GATE_CLOSE_THRESHOLD = 0.02  # below this -> eventually gate closes
GATE_OPEN_THRESHOLD  = 0.03  # above this -> gate opens

# Gate fade
GATE_FADE_SPEED = 0.3  # how quickly we move envelope (0.0 to 1.0) each chunk
GATE_CLOSED_LEVEL = 0.1  # how much to scale audio when fully closed

# Auto-gain
TARGET_RMS = 0.3
MAX_GAIN   = 10.0

# We'll keep track of whether the gate is logically open or closed
gate_is_open = False
# And a "gate envelope" from 0 (fully gated) to 1 (fully open)
gate_envelope = 0.0

# -------------------------------------------------------------------
# UDP setup
# -------------------------------------------------------------------
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
print(f"Listening on {UDP_IP}:{UDP_PORT}...")

samples = []
expected_samples = SAMPLE_RATE * RECORD_SECS

while len(samples) < expected_samples:
    data, addr = sock.recvfrom(8192)

    # Convert raw data to floats [-1..+1]
    chunk_list = []
    for i in range(0, len(data), 2):
        if i + 1 < len(data):
            adc_value = int.from_bytes(data[i:i+2], "little", signed=False)
            # If your ADC range is ~0..4095, midpoint ~2048:
            normalized = (adc_value - 2048) / 2048.0
            chunk_list.append(normalized)

    if not chunk_list:
        continue

    chunk = np.array(chunk_list, dtype=np.float32)

    # 1) Band-pass filtering
    filtered, filter_state = lfilter(b, a, chunk, zi=filter_state)

    # 2) Compute RMS of this chunk
    rms = np.sqrt(np.mean(filtered**2)) if len(filtered) else 0.0

    # 3) Hysteresis gate state update
    #    If gate is closed, we open only if RMS > GATE_OPEN_THRESHOLD
    #    If gate is open, we close only if RMS < GATE_CLOSE_THRESHOLD
    if gate_is_open:
        if rms < GATE_CLOSE_THRESHOLD:
            gate_is_open = False
    else:
        if rms > GATE_OPEN_THRESHOLD:
            gate_is_open = True

    # 4) Gradually move gate envelope toward 1 if open, else toward 0
    #    Envelope = 1 => fully open
    #    Envelope = 0 => fully closed
    target_envelope = 1.0 if gate_is_open else 0.0
    # Move gate_envelope by a fraction GATE_FADE_SPEED each chunk
    gate_envelope += (target_envelope - gate_envelope) * GATE_FADE_SPEED

    # 5) Combine gating with auto-gain
    #    - We'll do the normal auto-gain first, then apply gate envelope
    if rms > 1e-9:
        gain = TARGET_RMS / rms
        if gain > MAX_GAIN:
            gain = MAX_GAIN
    else:
        gain = 1.0

    # Apply gain
    filtered *= gain

    # Then apply gate envelope:
    # final_sample = envelope*(fully open) + (1 - envelope)*(fully closed level)
    # But we can interpret this in a simpler way:
    # - If envelope=1 => scale factor=1
    # - If envelope=0 => scale factor=GATE_CLOSED_LEVEL
    # - If envelope=0.5 => scale factor=0.5 + 0.5*GATE_CLOSED_LEVEL
    # ...
    gate_scale = gate_envelope + (1.0 - gate_envelope) * GATE_CLOSED_LEVEL
    filtered *= gate_scale

    # 6) Accumulate
    samples.extend(filtered.tolist())

sock.close()

# Trim if needed
if len(samples) < expected_samples:
    print(f"Warning: expected {expected_samples}, got {len(samples)}.")
elif len(samples) > expected_samples:
    samples = samples[:expected_samples]

output_array = np.array(samples, dtype=np.float32)
sf.write(OUTPUT_WAV, output_array, SAMPLE_RATE)
print(f"Saved {OUTPUT_WAV} with {len(output_array)} samples at {SAMPLE_RATE} Hz.")
