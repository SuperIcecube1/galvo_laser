

import tkinter as tk
from tkinter import messagebox
import sounddevice as sd
import socket
import threading
from http.server import ThreadingHTTPServer
import webbrowser
import requests
import json
import time
from threading import Thread
from multiprocessing import Value
from collections import deque
from scipy.signal import butter, lfilter
from http.server import BaseHTTPRequestHandler
import random
import numpy as np

# Audio stream configuration
CHUNK = 1024  # Chunk size for processing
RATE = 44100  # Sampling rate in Hz
DEVICE_INDEX = 1  # Default audio device index
CHANNELS = 2  # Stereo
DEFAULT_DEVICE_NAME = "BlackHole 2ch"  # Default audio device name

# Shared variables
spotify_bpm = Value('f', 120.0)
current_volume_level = Value('f', 0.0)
current_is_beat_drop = Value('i', 0)
current_energy_level = Value('i', -1)
beat_drop_timestamp = Value('d', 0.0)
last_high_energy_time = Value('d', 0.0)
current_mode = Value('i', 0)
strip_colors = [Value('i', 0xFFFFFF) for _ in range(4)]  # Stored as 0xRRGGBB
color_mode = Value('i', 0)

SPOTIFY_ACCESS_TOKEN = ''  # Will be set dynamically in the GUI


# Function to auto-detect the local IP address
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


# GUI class
class ServerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Server Configuration")

        # Server IP
        self.server_ip = tk.StringVar(value=get_local_ip())
        tk.Label(root, text="Server IP Address:").grid(row=0, column=0, padx=10, pady=5)
        self.ip_entry = tk.Entry(root, textvariable=self.server_ip, width=20)
        self.ip_entry.grid(row=0, column=1, padx=10, pady=5)

        # Audio Device Selection
        self.device_var = tk.StringVar()
        self.devices = sd.query_devices()
        self.device_names = [device['name'] for device in self.devices]
        default_device_idx = next((i for i, d in enumerate(self.device_names) if DEFAULT_DEVICE_NAME in d), 0)
        self.device_var.set(self.device_names[default_device_idx])

        tk.Label(root, text="Audio Device:").grid(row=1, column=0, padx=10, pady=5)
        self.device_menu = tk.OptionMenu(root, self.device_var, *self.device_names)
        self.device_menu.grid(row=1, column=1, padx=10, pady=5)

        # Spotify Client ID and Secret
        self.spotify_client_id = tk.StringVar(value='')
        self.spotify_client_secret = tk.StringVar(value='')
        tk.Label(root, text="Spotify Client ID:").grid(row=2, column=0, padx=10, pady=5)
        self.client_id_entry = tk.Entry(root, textvariable=self.spotify_client_id, width=50)
        self.client_id_entry.grid(row=2, column=1, padx=10, pady=5)

        tk.Label(root, text="Spotify Client Secret:").grid(row=3, column=0, padx=10, pady=5)
        self.client_secret_entry = tk.Entry(root, textvariable=self.spotify_client_secret, width=50)
        self.client_secret_entry.grid(row=3, column=1, padx=10, pady=5)

        # Start Server Button
        self.start_button = tk.Button(root, text="Start Server", command=self.start_server)
        self.start_button.grid(row=4, column=1, padx=10, pady=5)

        # Label to display the server link
        self.server_link_label = tk.Label(root, text="", fg="green", cursor="hand2")
        self.server_link_label.grid(row=5, column=1, padx=10, pady=5)

    def start_server(self):
        # Get IP, device, and token details from the GUI
        server_ip = self.server_ip.get()
        audio_device_name = self.device_var.get()
        client_id = self.spotify_client_id.get()
        client_secret = self.spotify_client_secret.get()

        # Ensure that Spotify credentials are optional
        if client_id == '' or client_secret == '':
            messagebox.showinfo("Info",
                                "Spotify Client ID or Secret not provided. The server will start without Spotify integration.")

        # Get device index from name
        device_index = next((i for i, d in enumerate(self.device_names) if d == audio_device_name), 0)
        global DEVICE_INDEX
        DEVICE_INDEX = device_index

        messagebox.showinfo("Info", "Starting the server with the selected configurations...")

        # Start the server in a new thread
        threading.Thread(target=self.run_server, args=(server_ip, client_id, client_secret), daemon=True).start()

    def run_server(self, ip_address, client_id, client_secret):
        # Start the audio analysis process
        audio_process = Thread(target=analyze_audio, args=(
        current_volume_level, current_is_beat_drop, current_energy_level, beat_drop_timestamp, last_high_energy_time))
        audio_process.start()

        # If Spotify credentials are provided, get the access token
        if client_id and client_secret:
            get_spotify_access_token(client_id, client_secret)

        # Start color mode management
        color_mode_thread = Thread(target=manage_color_modes)
        color_mode_thread.start()

        # Start the HTTP server
        server_url = f"http://{ip_address}:8080"
        httpd = ThreadingHTTPServer((ip_address, 8080), SimpleHTTPRequestHandler)
        print(f"Server started at {server_url}")

        # Update the GUI with the server link
        self.server_link_label.config(text=server_url)
        self.server_link_label.bind("<Button-1>", lambda e: webbrowser.open_new(server_url))

        httpd.serve_forever()


# Real-time audio analysis
def analyze_audio(shared_volume_level, shared_is_beat_drop, shared_energy_level, beat_drop_timestamp,
                  last_high_energy_time):
    last_volume_levels = deque(maxlen=int(RATE / CHUNK))

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
                shared_energy_level.value = -1

        with shared_volume_level.get_lock():
            shared_volume_level.value = volume

    try:
        with sd.InputStream(samplerate=RATE, channels=CHANNELS, device=DEVICE_INDEX, callback=audio_callback):
            while True:
                time.sleep(0.5)
    except Exception as e:
        print(f"Error starting audio stream: {e}")


# Function to get Spotify Access Token
def get_spotify_access_token(client_id, client_secret):
    token_url = "https://accounts.spotify.com/api/token"
    headers = {
        "Authorization": f"Basic {client_id}:{client_secret}",
        "Content-Type": "application/x-www-form-urlencoded"
    }
    data = {"grant_type": "client_credentials"}

    try:
        response = requests.post(token_url, headers=headers, data=data)
        response_json = response.json()
        global SPOTIFY_ACCESS_TOKEN
        SPOTIFY_ACCESS_TOKEN = response_json.get("access_token")
        print(f"Spotify Access Token: {SPOTIFY_ACCESS_TOKEN}")
    except Exception as e:
        print(f"Error getting Spotify access token: {e}")


# Function to handle color changes
def manage_color_modes():
    current_colors = [strip_colors[i].value for i in range(4)]
    target_colors = current_colors.copy()
    hue = 0

    while True:
        if color_mode.value == 1:
            hue = (hue + 20) % 256
            new_rgb = hsv_to_rgb(hue, 255, 255)
            target_colors = [new_rgb] * 4
        elif color_mode.value == 2:
            color1 = hsv_to_rgb(random.randint(0, 255), 255, 255)
            color2 = hsv_to_rgb(random.randint(0, 255), 255, 255)
            if random.choice([True, False]):
                target_colors[0] = target_colors[2] = color1
                target_colors[1] = target_colors[3] = color2
            else:
                target_colors[0] = target_colors[3] = color1
                target_colors[1] = target_colors[2] = color2
        elif color_mode.value == 3:
            halloween_colors = [0xFF4500, 0x8A2BE2, 0xFF0000, 0x008000]
            pairing = random.choice([(0, 2, 1, 3), (0, 3, 1, 2)])
            for i in range(2):
                color = halloween_colors[random.randint(0, len(halloween_colors) - 1)]
                target_colors[pairing[i * 2]] = target_colors[pairing[i * 2 + 1]] = color
        elif color_mode.value == 4:
            boiler_colors = [0x8B0000, 0x4B0082]
            pairing = random.choice([(0, 2, 1, 3), (0, 3, 1, 2)])
            for i in range(2):
                color = boiler_colors[random.randint(0, len(boiler_colors) - 1)]
                target_colors[pairing[i * 2]] = target_colors[pairing[i * 2 + 1]] = color
        for step in range(100):
            for i in range(4):
                current_colors[i] = interpolate_color(current_colors[i], target_colors[i], step / 100.0)
                with strip_colors[i].get_lock():
                    strip_colors[i].value = current_colors[i]
            time.sleep(0.01)


# Helper function for color conversion
def hsv_to_rgb(h, s, v):
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
    r1, g1, b1 = (color1 >> 16) & 0xFF, (color1 >> 8) & 0xFF, color1 & 0xFF
    r2, g2, b2 = (color2 >> 16) & 0xFF, (color2 >> 8) & 0xFF, color2 & 0xFF
    r = int(r1 + (r2 - r1) * t)
    g = int(g1 + (g2 - g1) * t)
    b = int(b1 + (b2 - b1) * t)
    return (r << 16) | (g << 8) | b


# HTTP server handler
class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            with open('index.html', 'r') as f:
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


# Main application loop
if __name__ == '__main__':
    root = tk.Tk()
    gui = ServerGUI(root)
    root.mainloop()