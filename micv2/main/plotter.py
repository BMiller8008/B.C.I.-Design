import serial
import matplotlib.pyplot as plt
from collections import deque

# Update the serial port to match your actual port
SERIAL_PORT = 'COM10'
BAUD_RATE = 115200
BUFFER_SIZE = 500  # This is more reasonable for real-time plotting (adjust if needed)

# Open serial connection
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Initialize the plot
plt.ion()
fig, ax = plt.subplots()
pcm_buffer = deque([0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)

line, = ax.plot(range(BUFFER_SIZE), pcm_buffer)
ax.set_ylim(-32768, 32767)  # Full range for 16-bit signed PCM
ax.set_title('Filtered PCM Output from MAX4466 Microphone')
ax.set_xlabel('Sample')
ax.set_ylabel('PCM Value')

# Main loop to read serial data and update plot
try:
    while True:
        if ser.in_waiting > 0:
            line_data = ser.readline().decode('utf-8', errors='ignore').strip()
            try:
                pcm_value = int(line_data)  # Parse as integer
                pcm_buffer.append(pcm_value)

                # Update plot
                line.set_ydata(pcm_buffer)
                ax.relim()
                ax.autoscale_view()
                plt.pause(0.01)

            except ValueError:
                print(f"Skipping invalid data: {line_data}")  # Catch non-numeric data if noise gets through

except KeyboardInterrupt:
    print("Stopped by user.")
    ser.close()
    plt.ioff()
    plt.show()  # Final display after stopping
