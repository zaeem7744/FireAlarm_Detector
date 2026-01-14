import serial
import numpy as np
import wave
import threading
import tkinter as tk
from tkinter import filedialog
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import time

PORT = "COM13"
BAUD = 921600  # must match Serial.begin on ESP32
SAMPLE_RATE = 16000
SAMPLES_PER_PACKET = 512  # MUST match ESP32!
BYTES_PER_PACKET = SAMPLES_PER_PACKET * 2  # 16-bit = 2 bytes per sample

# Rolling display window (e.g. last 1 second = 32 packets * 512 samples / 16kHz)
DISPLAY_PACKETS = 32
DISPLAY_SAMPLES = DISPLAY_PACKETS * SAMPLES_PER_PACKET

print(f"Expecting {BYTES_PER_PACKET} bytes per packet")

# Open serial with buffer
ser = serial.Serial(PORT, BAUD, timeout=0.5)
ser.reset_input_buffer()

recording = False
audio_buffer = []
stream_buffer = np.zeros(SAMPLES_PER_PACKET, dtype=np.int16)
# display_buffer holds a rolling window of the last DISPLAY_SAMPLES samples
display_buffer = np.zeros(DISPLAY_SAMPLES, dtype=np.int16)
packet_count = 0

def serial_reader():
    global audio_buffer, recording, stream_buffer, display_buffer, packet_count
    
    while True:
        try:
            # Wait for header
            header = ser.read(2)
            if len(header) != 2:
                continue
                
            if header[0] != 0xAA or header[1] != 0x55:
                # Skip bad header
                continue
            
            # Read EXACT audio packet size
            data = ser.read(BYTES_PER_PACKET)
            
            if len(data) != BYTES_PER_PACKET:
                print(f"Bad packet: got {len(data)} bytes, expected {BYTES_PER_PACKET}")
                continue
            
            # Convert to 16-bit samples
            samples = np.frombuffer(data, dtype=np.int16)
            stream_buffer = samples.copy()

            # Update rolling display buffer (real-time waveform)
            # Shift left and append new samples at the end
            if len(samples) == SAMPLES_PER_PACKET:
                display_buffer = np.roll(display_buffer, -SAMPLES_PER_PACKET)
                display_buffer[-SAMPLES_PER_PACKET:] = samples
            
            packet_count += 1
            if packet_count % 100 == 0:
                print(f"Packet {packet_count}, RMS: {np.sqrt(np.mean(samples.astype(float)**2)):.1f}")
            
            if recording:
                audio_buffer.extend(samples)
                
        except Exception as e:
            msg = str(e)
            # On Windows with some USB CDC drivers, pyserial may raise noisy
            # "ClearCommError failed" PermissionError even though streaming works.
            # Suppress repeated logging for that specific case.
            if "ClearCommError failed" in msg:
                time.sleep(0.1)
                continue

            print(f"Serial error: {e}")
            time.sleep(0.1)

# Start thread
thread = threading.Thread(target=serial_reader, daemon=True)
thread.start()

# GUI
root = tk.Tk()
root.title("INMP441 Audio Recorder - 16kHz")
root.geometry("900x600")

# Status
status = tk.StringVar()
status.set("Ready - Waiting for audio...")

def start_record():
    global recording, audio_buffer
    audio_buffer = []
    recording = True
    status.set("Recording...")
    print("Recording started")

def stop_record():
    global recording
    recording = False
    
    if len(audio_buffer) == 0:
        status.set("No audio recorded!")
        return
    
    status.set("Saving...")
    root.update()
    
    filename = filedialog.asksaveasfilename(
        defaultextension=".wav",
        filetypes=[("WAV files", "*.wav"), ("All files", "*.*")]
    )
    
    if filename:
        try:
            # Calculate actual duration
            duration = len(audio_buffer) / SAMPLE_RATE
            print(f"Saving {len(audio_buffer)} samples ({duration:.2f} seconds)")
            
            # Save WAV
            wav = wave.open(filename, "w")
            wav.setnchannels(1)
            wav.setsampwidth(2)  # 16-bit = 2 bytes
            wav.setframerate(SAMPLE_RATE)
            wav.writeframes(np.array(audio_buffer, dtype=np.int16).tobytes())
            wav.close()
            
            status.set(f"Saved: {filename}")
            print(f"Saved to {filename}")
            
        except Exception as e:
            status.set(f"Error: {str(e)}")
            print(f"Save error: {e}")
    else:
        status.set("Save cancelled")

# Layout
tk.Label(root, text="INMP441 Fire Alarm Recorder", font=("Arial", 16, "bold")).pack(pady=10)

button_frame = tk.Frame(root)
button_frame.pack(pady=10)

tk.Button(button_frame, text="🎤 Start Recording", bg="green", fg="white", 
          font=("Arial", 12), width=20, height=2, command=start_record).pack(side=tk.LEFT, padx=10)

tk.Button(button_frame, text="⏹ Stop & Save", bg="red", fg="white",
          font=("Arial", 12), width=20, height=2, command=stop_record).pack(side=tk.LEFT, padx=10)

tk.Label(root, textvariable=status, font=("Arial", 12), fg="blue").pack(pady=10)

# Plots
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6))
fig.tight_layout(pad=4.0)
fig.patch.set_facecolor('#f0f0f0')

canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=20, pady=10)

def update_plots():
    ax1.clear()
    ax2.clear()
    
    # Waveform: show last DISPLAY_SAMPLES samples as a continuous window (~1s)
    ax1.plot(display_buffer, color='blue', linewidth=1)
    ax1.set_title(f"Live Audio - last {DISPLAY_SAMPLES / SAMPLE_RATE:.2f}s (packet {packet_count})", fontsize=12)
    ax1.set_xlabel("Sample index (rolling window)")
    ax1.set_ylabel("Amplitude")
    # Auto-scale around current data range with some headroom
    if np.max(np.abs(display_buffer)) > 0:
        max_abs = np.max(np.abs(display_buffer))
        ax1.set_ylim(-1.2 * max_abs, 1.2 * max_abs)
    else:
        ax1.set_ylim(-1000, 1000)
    ax1.grid(True, alpha=0.3)
    
    # Spectrum of most recent packet
    if len(stream_buffer) > 256:
        fft = np.abs(np.fft.rfft(stream_buffer[:256]))
        freq = np.fft.rfftfreq(256, 1/SAMPLE_RATE)
        ax2.plot(freq, fft, color='green', linewidth=1)
        ax2.set_title("Frequency Spectrum", fontsize=12)
        ax2.set_xlabel("Frequency (Hz)")
        ax2.set_ylabel("Magnitude")
        ax2.set_xlim(0, 4000)  # Show up to 4kHz
        ax2.grid(True, alpha=0.3)
    
    canvas.draw()
    root.after(50, update_plots)  # Update every 50ms for smoother real-time view

update_plots()
root.mainloop()