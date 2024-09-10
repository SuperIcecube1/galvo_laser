import threading  # Make sure threading is imported
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
import os


# Audio stream configuration
CHUNK = 1024  # Chunk size for processing
RATE = 44100  # Sampling rate in Hz
DEVICE_INDEX = 0  # Change this to the correct device index for your audio device
CHANNELS = 2  # Assuming stereo input

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

# Spotify API credentials
SPOTIFY_ACCESS_TOKEN = 'your_spotify_access_token'
SPOTIFY_API_URL = 'https://api.spotify.com/v1/me/player/currently-playing'

# Bandpass filter for focusing on specific frequency ranges
def butter_bandpass(lowcut, highcut, fs, order=5):
    nyquist = 0.5 * fs
    low = lowcut / nyquist
    high = highcut / nyquist
    b, a = butter(order, [low, high], btype='band')
    return b, a

def bandpass_filter(data, lowcut, highcut, fs, order=5):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

# Real-time audio analysis
def analyze_audio(shared_volume_level, shared_is_beat_drop, shared_energy_level, beat_drop_timestamp, last_high_energy_time):
    last_volume_levels = deque(maxlen=int(RATE / CHUNK))  # Store volume levels for the last second

    def audio_callback(indata, frames, time_info, status):
        if status:
            print(status)

        audio_data = np.mean(indata, axis=1)
        volume = np.mean(np.abs(audio_data)) * 2

        last_volume_levels.append(volume)
        avg_last_volume = np.mean(last_volume_levels)

        current_time = time.time()
        time_since_last_beat_drop = current_time - beat_drop_timestamp.value

        with shared_is_beat_drop.get_lock():
            if (volume - avg_last_volume > 0.3) and volume > 0.35 and time_since_last_beat_drop >= 10:
                shared_is_beat_drop.value = 1
                with beat_drop_timestamp.get_lock():
                    beat_drop_timestamp.value = current_time
            elif time_since_last_beat_drop <= 1:
                shared_is_beat_drop.value = 1
            else:
                shared_is_beat_drop.value = 0

        with last_high_energy_time.get_lock():
            if volume > 0.30:
                shared_energy_level.value = 3
                last_high_energy_time.value = current_time
            elif volume > 0.20:
                shared_energy_level.value = 2
            elif volume > 0.10:
                shared_energy_level.value = 1
            elif volume > 0.01:
                shared_energy_level.value = 0
            else:
                shared_energy_level.value = -1  # No sound

        with shared_volume_level.get_lock():
            shared_volume_level.value = volume

    with sd.InputStream(samplerate=RATE, channels=CHANNELS, device=DEVICE_INDEX, callback=audio_callback):
        while True:
            time.sleep(0.5)

# Function to fetch BPM from Spotify
def fetch_spotify_bpm(shared_spotify_bpm):
    headers = {
        "Authorization": f"Bearer {SPOTIFY_ACCESS_TOKEN}"
    }

    while True:
        try:
            response = requests.get(SPOTIFY_API_URL, headers=headers, timeout=5)
            data = response.json()

            track_id = data['item']['id']
            track_features_url = f"https://api.spotify.com/v1/audio-features/{track_id}"
            response = requests.get(track_features_url, headers=headers, timeout=5)
            track_data = response.json()

            bpm = track_data['tempo']
            with shared_spotify_bpm.get_lock():
                shared_spotify_bpm.value = bpm

            print(f"Fetched BPM from Spotify: {bpm}")
        except requests.exceptions.RequestException as e:
            print(f"Error fetching BPM from Spotify: {e}")
        except Exception as e:
            print(f"General error: {e}")

        time.sleep(20)

# Function to handle color changes
def manage_color_modes():
    current_colors = [strip_colors[i].value for i in range(4)]
    target_colors = current_colors.copy()
    hue = 0

    while True:
        if color_mode.value == 1:  # Full spectrum mode
            hue = (hue + 20) % 256
            new_rgb = hsv_to_rgb(hue, 255, 255)
            target_colors = [new_rgb] * 4

        elif color_mode.value == 2:  # Rave mode with random jumps across the spectrum
            color1 = hsv_to_rgb(random.randint(0, 255), 255, 255)
            color2 = hsv_to_rgb(random.randint(0, 255), 255, 255)

            if random.choice([True, False]):
                target_colors[0] = target_colors[2] = color1  # Strips 1 and 3
                target_colors[1] = target_colors[3] = color2  # Strips 2 and 4
            else:
                target_colors[0] = target_colors[3] = color1  # Strips 1 and 4
                target_colors[1] = target_colors[2] = color2  # Strips 2 and 3

        elif color_mode.value == 3:  # Halloween mode
            halloween_colors = [0xFF4500, 0x8A2BE2, 0xFF0000, 0x008000]  # Orange, Purple, Red, Green
            pairing = random.choice([(0, 2, 1, 3), (0, 3, 1, 2)])  # Randomly choose pairing for the strips
            for i in range(2):
                color = halloween_colors[random.randint(0, len(halloween_colors) - 1)]
                target_colors[pairing[i * 2]] = target_colors[pairing[i * 2 + 1]] = color

        elif color_mode.value == 4:  # Boiler Room mode
            boiler_colors = [0x8B0000, 0x4B0082]  # Dark Red, Indigo
            pairing = random.choice([(0, 2, 1, 3), (0, 3, 1, 2)])  # Randomly choose pairing for the strips
            for i in range(2):
                color = boiler_colors[random.randint(0, len(boiler_colors) - 1)]
                target_colors[pairing[i * 2]] = target_colors[pairing[i * 2 + 1]] = color

        # Gradually transition from current to target colors
        for step in range(100):
            for i in range(4):
                current_colors[i] = interpolate_color(current_colors[i], target_colors[i], step / 100.0)
                with strip_colors[i].get_lock():
                    strip_colors[i].value = current_colors[i]
            time.sleep(0.01)

def hsv_to_rgb(h, s, v):
    """Convert HSV to RGB color."""
    h = float(h)
    s = float(s) / 255.0
    v = float(v) / 255.0
    hi = int(h / 60.0) % 6
    f = (h / 60.0) - hi
    p = v * (1.0 - s)
    q = v * (1.0 - f * s)
    t = v * (1.0 - (1.0 - f) * s)
    r, g, b = 0, 0, 0
    if hi == 0:
        r, g, b = v, t, p
    elif hi == 1:
        r, g, b = q, v, p
    elif hi == 2:
        r, g, b = p, v, t
    elif hi == 3:
        r, g, b = p, q, v
    elif hi == 4:
        r, g, b = t, p, v
    elif hi == 5:
        r, g, b = v, p, q
    return (int(r * 255) << 16) | (int(g * 255) << 8) | int(b * 255)

def interpolate_color(color1, color2, t):
    """Interpolate between two colors with a ratio t (0.0 - 1.0)."""
    r1, g1, b1 = (color1 >> 16) & 0xFF, (color1 >> 8) & 0xFF, color1 & 0xFF
    r2, g2, b2 = (color2 >> 16) & 0xFF, (color2 >> 8) & 0xFF, color2 & 0xFF
    r = int(r1 + (r2 - r1) * t)
    g = int(g1 + (g2 - g1) * t)
    b = int(b1 + (b2 - b1) * t)
    return (r << 16) | (g << 8) | b

# Tkinter GUI Class
class AudioVisualizerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Audio Visualizer Controller")

        # Variables
        self.server_ip = tk.StringVar(value="http://127.0.0.1:8080")  # Default IP for local server
        self.spotify_bpm = tk.DoubleVar(value=120.0)
        self.volume_level = tk.DoubleVar(value=0.0)
        self.energy_level = tk.StringVar(value="No Sound")
        self.is_beat_drop = tk.StringVar(value="No")
        self.colors = ["#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF"]

        # IP Address Input
        tk.Label(root, text="Webserver IP:").grid(row=0, column=0, padx=10, pady=5, sticky="w")
        self.ip_entry = tk.Entry(root, textvariable=self.server_ip, width=20)
        self.ip_entry.grid(row=0, column=1, padx=10, pady=5, sticky="w")

        # Display status
        self.status_label = tk.Label(root, text="Status: ", font=('Helvetica', 12))
        self.status_label.grid(row=1, column=0, columnspan=3, pady=10)

        # Data Display
        tk.Label(root, text="Spotify BPM:").grid(row=2, column=0, padx=10, pady=5, sticky="w")
        self.bpm_label = tk.Label(root, textvariable=self.spotify_bpm)
        self.bpm_label.grid(row=2, column=1, padx=10, pady=5, sticky="w")

        tk.Label(root, text="Volume Level:").grid(row=3, column=0, padx=10, pady=5, sticky="w")
        self.volume_label = tk.Label(root, textvariable=self.volume_level)
        self.volume_label.grid(row=3, column=1, padx=10, pady=5, sticky="w")

        tk.Label(root, text="Energy Level:").grid(row=4, column=0, padx=10, pady=5, sticky="w")
        self.energy_label = tk.Label(root, textvariable=self.energy_level)
        self.energy_label.grid(row=4, column=1, padx=10, pady=5, sticky="w")

        tk.Label(root, text="Beat Drop:").grid(row=5, column=0, padx=10, pady=5, sticky="w")
        self.beat_drop_label = tk.Label(root, textvariable=self.is_beat_drop)
        self.beat_drop_label.grid(row=5, column=1, padx=10, pady=5, sticky="w")

        # Start updating the GUI
        self.update_gui()

    def update_gui(self):
        def fetch_data():
            try:
                response = requests.get(f"{self.server_ip.get()}/status")
                data = response.json()

                # Update the GUI from the fetched data
                self.spotify_bpm.set(data['spotify_bpm'])
                self.volume_level.set(data['volume_level'])
                self.energy_level.set(
                    "High" if data['energy_level'] == 3 else
                    "Medium" if data['energy_level'] == 2 else
                    "Low" if data['energy_level'] == 1 else
                    "No Sound"
                )
                self.is_beat_drop.set("Yes" if data['is_beat_drop'] else "No")

                self.status_label.config(text="Connected to server")

            except Exception as e:
                self.status_label.config(text=f"Error: {e}")

        threading.Thread(target=fetch_data, daemon=True).start()
        self.root.after(1000, self.update_gui)

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
    httpd = ThreadingHTTPServer(('0.0.0.0', 8080), SimpleHTTPRequestHandler)
    print("Server started at http://0.0.0.0:8080")
    httpd.serve_forever()

# Function to start the GUI
def start_gui():
    root = tk.Tk()
    gui = AudioVisualizerGUI(root)
    root.mainloop()

# Main function
if __name__ == '__main__':
    # Start the web server in a separate thread
    server_thread = threading.Thread(target=start_server)  # Use threading.Thread
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

    # Start the Tkinter GUI in the main thread
    start_gui()

    audio_process.join()
    spotify_thread.join()
    color_mode_thread.join()

