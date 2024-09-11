import threading
import numpy as np
import sounddevice as sd
import requests
from scipy.signal import butter, lfilter
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import time
from collections import deque
from multiprocessing import Value
import tkinter as tk
import random
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import socket

# Audio stream configuration
CHUNK = 1024  # Chunk size for processing
RATE = 44100  # Sampling rate in Hz
DEVICE_INDEX = 0  # Default audio device index (set to 0 to avoid issues with unavailable devices)
CHANNELS = 2  # Stereo

# Shared variables
spotify_bpm = Value('f', 120.0)  # BPM from Spotify
current_volume_level = Value('f', 0.0)
current_is_beat_drop = Value('i', 0)  # 0 = No Beat Drop, 1 = Beat Drop
current_energy_level = Value('i', -1)  # -1 = No Sound, 0 = No Energy, 1 = Low Energy, 2 = Normal Energy, 3 = High Energy
beat_drop_timestamp = Value('d', 0.0)  # Timestamp of the last detected beat drop
last_high_energy_time = Value('d', 0.0)  # Timestamp of the last detected high energy
current_mode = Value('i', 0)  # 0 = None, 1-3 for scenes, 4-13 for effects

# RGB Color values for each strip
strip_colors = [Value('i', 0xFFFFFF) for _ in range(4)]  # Stored as 0xRRGGBB
color_mode = Value('i', 0)  # 0 = static, 1 = full spectrum, 2 = rave, 3 = halloween, 4 = boiler_room

# Automatically get the local IP address of the Mac
def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip


SERVER_IP = get_local_ip()


# Real-time audio analysis (uses sounddevice to capture audio)
def analyze_audio(shared_volume_level, shared_is_beat_drop, shared_energy_level, beat_drop_timestamp, last_high_energy_time):
    # Store volume levels for the last second
    last_volume_levels = deque(maxlen=int(RATE / CHUNK))  # Store volume levels for the last second

    def audio_callback(indata, frames, time_info, status):
        if status:
            print(f"Audio callback status: {status}")  # Print any status warnings/errors

        audio_data = np.mean(np.abs(indata))  # Process raw sound amplitude
        print(f"Volume Level: {audio_data}")  # Print current volume level for debugging

        last_volume_levels.append(audio_data)
        avg_last_volume = np.mean(last_volume_levels)
        current_time = time.time()
        time_since_last_beat_drop = current_time - beat_drop_timestamp.value

        with shared_is_beat_drop.get_lock():
            if (audio_data - avg_last_volume > 0.3) and audio_data > 0.35 and time_since_last_beat_drop >= 10:
                shared_is_beat_drop.value = 1
                with beat_drop_timestamp.get_lock():
                    beat_drop_timestamp.value = current_time
            elif time_since_last_beat_drop <= 1:
                shared_is_beat_drop.value = 1
            else:
                shared_is_beat_drop.value = 0

        with last_high_energy_time.get_lock():
            if audio_data > 0.30:
                shared_energy_level.value = 3
                last_high_energy_time.value = current_time
            elif audio_data > 0.20:
                shared_energy_level.value = 2
            elif audio_data > 0.10:
                shared_energy_level.value = 1
            elif audio_data > 0.01:
                shared_energy_level.value = 0
            else:
                shared_energy_level.value = -1  # No sound

        with shared_volume_level.get_lock():
            shared_volume_level.value = audio_data

    # Print the selected audio input device info
    try:
        device_info = sd.query_devices(DEVICE_INDEX)
        print(f"Using audio input device: {device_info['name']} (Index: {DEVICE_INDEX})")  # Print current device information
        print(f"Sample Rate: {RATE}, Channels: {CHANNELS}")

        # Start the audio stream with the proper device index
        with sd.InputStream(samplerate=RATE, channels=CHANNELS, device=DEVICE_INDEX, callback=audio_callback):
            print("Audio stream started successfully.")
            while True:
                time.sleep(0.5)  # Keep the stream alive
    except Exception as e:
        print(f"Error starting audio stream: {e}")


# Tkinter GUI Class with audio device selection
class AudioVisualizerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Audio Visualizer Controller")

        # Variables
        self.server_ip = tk.StringVar(value=f"http://{SERVER_IP}:8080")  # Set IP dynamically based on Mac's current IP
        self.spotify_bpm = tk.DoubleVar(value=120.0)
        self.volume_level = tk.DoubleVar(value=0)  # Corrected here
        self.energy_level = tk.StringVar(value="No Sound")
        self.is_beat_drop = tk.StringVar(value="No")
        self.colors = ["#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF"]

        self.volume_data = deque(maxlen=100)  # Data points limit for the graph

        # Get available audio devices
        self.devices = sd.query_devices()
        self.device_names = [device['name'] for device in self.devices]

        # IP Address Input
        tk.Label(root, text="Webserver IP:").grid(row=0, column=0, padx=10, pady=5, sticky="w")
        self.ip_entry = tk.Entry(root, textvariable=self.server_ip, width=20)
        self.ip_entry.grid(row=0, column=1, padx=10, pady=5, sticky="w")

        # Audio Device Selection
        tk.Label(root, text="Audio Device:").grid(row=1, column=0, padx=10, pady=5, sticky="w")
        self.device_var = tk.StringVar(value=self.device_names[DEVICE_INDEX])
        self.device_menu = tk.OptionMenu(root, self.device_var, *self.device_names, command=self.change_device)
        self.device_menu.grid(row=1, column=1, padx=10, pady=5)

        # Start Server Button
        self.start_server_button = tk.Button(root, text="Start Server", command=self.start_server)
        self.start_server_button.grid(row=0, column=2, padx=10, pady=5)

        # Display status
        self.status_label = tk.Label(root, text="Status: ", font=('Helvetica', 12))
        self.status_label.grid(row=2, column=0, columnspan=3, pady=10)

        # Data Display
        tk.Label(root, text="Spotify BPM:").grid(row=3, column=0, padx=10, pady=5, sticky="w")
        self.bpm_label = tk.Label(root, textvariable=self.spotify_bpm)
        self.bpm_label.grid(row=3, column=1, padx=10, pady=5, sticky="w")

        tk.Label(root, text="Volume Level:").grid(row=4, column=0, padx=10, pady=5, sticky="w")
        self.volume_label = tk.Label(root, textvariable=self.volume_level)
        self.volume_label.grid(row=4, column=1, padx=10, pady=5, sticky="w")

        tk.Label(root, text="Energy Level:").grid(row=5, column=0, padx=10, pady=5, sticky="w")
        self.energy_label = tk.Label(root, textvariable=self.energy_level)
        self.energy_label.grid(row=5, column=1, padx=10, pady=5, sticky="w")

        tk.Label(root, text="Beat Drop:").grid(row=6, column=0, padx=10, pady=5, sticky="w")
        self.beat_drop_label = tk.Label(root, textvariable=self.is_beat_drop)
        self.beat_drop_label.grid(row=6, column=1, padx=10, pady=5, sticky="w")

        # Graph setup
        self.fig = Figure(figsize=(5, 3), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_title("Volume Level Over Time")
        self.ax.set_ylim(0, 1)  # Set y-axis limits based on expected volume level
        self.line, = self.ax.plot([], [], 'b-')

        self.canvas = FigureCanvasTkAgg(self.fig, master=root)
        self.canvas.get_tk_widget().grid(row=7, column=0, columnspan=2, pady=10)

        # Start updating the GUI
        self.update_gui()

    def update_gui(self):
        with current_volume_level.get_lock():
            volume = current_volume_level.value
        with current_energy_level.get_lock():
            energy = current_energy_level.value
        with current_is_beat_drop.get_lock():
            beat_drop = current_is_beat_drop.value

        self.volume_level.set(volume)
        self.energy_level.set(
            "High" if energy == 3 else
            "Medium" if energy == 2 else
            "Low" if energy == 1 else
            "No Sound"
        )
        self.is_beat_drop.set("Yes" if beat_drop else "No")

        # Update the graph
        self.volume_data.append(volume)
        self.line.set_xdata(range(len(self.volume_data)))
        self.line.set_ydata(self.volume_data)
        self.ax.set_xlim(0, len(self.volume_data))
        self.canvas.draw()

        # Continue updating every second
        self.root.after(1000, self.update_gui)

    def start_server(self):
        threading.Thread(target=start_server, daemon=True).start()

    def change_device(self, device_name):
        global DEVICE_INDEX
        DEVICE_INDEX = self.device_names.index(device_name)
        print(f"Changed audio input to: {device_name}")

# HTTP Server to display the analysis results
class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            with open('index.html', 'r') as f:  # Serve the separate HTML file
                self.wfile.write(f.read().encode('utf-8'))

        elif self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            with spotify_bpm.get_lock():
                sp_bpm = spotify_bpm.value
            with current_volume_level.get_lock():
                volume_level = current_volume_level.value
            with current_is_beat_drop.get_lock():
                is_beat_drop = current_is_beat_drop.value
            with current_energy_level.get_lock():
                energy_level = current_energy_level.value
            with current_mode.get_lock():
                mode = current_mode.value

            colors = []
            for i in range(4):
                with strip_colors[i].get_lock():
                    colors.append('#{:06x}'.format(strip_colors[i].value))

            response = json.dumps({
                'spotify_bpm': sp_bpm,
                'volume_level': volume_level,
                'is_beat_drop': is_beat_drop,
                'energy_level': energy_level,
                'current_mode': mode,
                'colors': colors
            })
            self.wfile.write(response.encode())

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)

        try:
            data = json.loads(post_data)
            response = {"status": "success"}

            if self.path == '/set-mode':
                mode = data['mode']
                with current_mode.get_lock():
                    current_mode.value = mode
                print(f"Mode received: {mode}")

            elif self.path == '/set-color':
                mode = data['mode']
                if mode == 'static':
                    colors = data['colors']
                    for i in range(4):
                        r = int(colors[i][1:3], 16)
                        g = int(colors[i][3:5], 16)
                        b = int(colors[i][5:7], 16)
                        with strip_colors[i].get_lock():
                            strip_colors[i].value = (r << 16) | (g << 8) | b
                    with color_mode.get_lock():
                        color_mode.value = 0
                    print(f"Colors received: {colors}")
                elif mode == 'full_spectrum':
                    with color_mode.get_lock():
                        color_mode.value = 1
                    print("Full spectrum mode activated")
                elif mode == 'rave':
                    with color_mode.get_lock():
                        color_mode.value = 2
                    print("Rave mode activated")
                elif mode == 'halloween':
                    with color_mode.get_lock():
                        color_mode.value = 3
                    print("Halloween mode activated")
                elif mode == 'boiler_room':
                    with color_mode.get_lock():
                        color_mode.value = 4
                    print("Boiler Room mode activated")
                else:
                    response = {"status": "error", "message": "Invalid color mode"}

            else:
                response = {"status": "error", "message": "Invalid endpoint"}

            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(response).encode())

        except json.JSONDecodeError:
            self.send_response(400)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({"status": "error", "message": "Invalid JSON"}).encode())


# Function to start the HTTP server
def start_server():
    httpd = ThreadingHTTPServer((SERVER_IP, 8080), SimpleHTTPRequestHandler)
    print(f"Server started at http://{SERVER_IP}:8080")
    httpd.serve_forever()


# Main function
if __name__ == '__main__':
    root = tk.Tk()
    gui = AudioVisualizerGUI(root)
    root.mainloop()

    # Start the web server in a separate thread
    server_thread = threading.Thread(target=start_server)
    server_thread.start()

    # Start the audio analysis process
    audio_process = threading.Thread(target=analyze_audio, args=(current_volume_level, current_is_beat_drop, current_energy_level, beat_drop_timestamp, last_high_energy_time))
    audio_process.start()

    # Start the Spotify BPM fetching process
    spotify_thread = threading.Thread(target=fetch_spotify_bpm, args=(spotify_bpm,))
    spotify_thread.start()

    # Start color mode management
    color_mode_thread = threading.Thread(target=manage_color_modes)
    color_mode_thread.start()
