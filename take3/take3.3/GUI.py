import tkinter as tk
import requests
import threading
import time

class AudioVisualizerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Audio Visualizer Controller")

        # Variables
        self.server_ip = tk.StringVar(value="192.168.1.131:8080")  # Default webserver IP
        self.spotify_bpm = tk.DoubleVar(value=120.0)
        self.volume_level = tk.DoubleVar(value=0.0)
        self.energy_level = tk.StringVar(value="No Sound")
        self.is_beat_drop = tk.StringVar(value="No")
        self.colors = ["#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF"]

        # IP Address Input
        tk.Label(root, text="Webserver IP:").grid(row=0, column=0, padx=10, pady=5, sticky="w")
        self.ip_entry = tk.Entry(root, textvariable=self.server_ip, width=20)
        self.ip_entry.grid(row=0, column=1, padx=10, pady=5, sticky="w")

        self.run_button = tk.Button(root, text="Run Webserver", command=self.run_webserver)
        self.run_button.grid(row=0, column=2, padx=10, pady=5, sticky="w")

        # Status Display
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

        # Color Picker
        self.color_pickers = []
        for i in range(4):
            tk.Label(root, text=f"Strip {i+1} Color:").grid(row=6+i, column=0, padx=10, pady=5, sticky="w")
            color_picker = tk.Entry(root, width=10)
            color_picker.insert(0, "#FFFFFF")
            color_picker.grid(row=6+i, column=1, padx=10, pady=5, sticky="w")
            self.color_pickers.append(color_picker)

        tk.Button(root, text="Set Colors", command=self.set_colors).grid(row=10, column=0, padx=10, pady=5)
        tk.Button(root, text="Full Spectrum Mode", command=self.set_full_spectrum).grid(row=10, column=1, padx=10, pady=5)
        tk.Button(root, text="Rave Mode", command=self.set_rave).grid(row=11, column=0, padx=10, pady=5)
        tk.Button(root, text="Halloween Mode", command=self.set_halloween).grid(row=11, column=1, padx=10, pady=5)
        tk.Button(root, text="Boiler Room Mode", command=self.set_boiler_room).grid(row=12, column=0, padx=10, pady=5)

        self.update_gui()

    def run_webserver(self):
        self.server_url = f"http://{self.server_ip.get()}/status"
        self.status_label.config(text=f"Connected to server at {self.server_ip.get()}")

    def update_gui(self):
        if hasattr(self, 'server_url'):
            try:
                response = requests.get(self.server_url)
                data = response.json()
                self.spotify_bpm.set(data['spotify_bpm'])
                self.volume_level.set(data['volume_level'])
                self.energy_level.set("High" if data['energy_level'] == 3 else "Medium" if data['energy_level'] == 2 else "Low" if data['energy_level'] == 1 else "No Sound")
                self.is_beat_drop.set("Yes" if data['is_beat_drop'] else "No")
                for i, color_picker in enumerate(self.color_pickers):
                    color_picker.delete(0, tk.END)
                    color_picker.insert(0, data['colors'][i])
            except Exception as e:
                self.status_label.config(text=f"Error: {e}")

        self.root.after(1000, self.update_gui)

    def set_colors(self):
        colors = [picker.get() for picker in self.color_pickers]
        payload = {"mode": "static", "colors": colors}
        requests.post(f"http://{self.server_ip.get()}/set-color", json=payload)

    def set_full_spectrum(self):
        requests.post(f"http://{self.server_ip.get()}/set-color", json={"mode": "full_spectrum"})

    def set_rave(self):
        requests.post(f"http://{self.server_ip.get()}/set-color", json={"mode": "rave"})

    def set_halloween(self):
        requests.post(f"http://{self.server_ip.get()}/set-color", json={"mode": "halloween"})

    def set_boiler_room(self):
        requests.post(f"http://{self.server_ip.get()}/set-color", json={"mode": "boiler_room"})

if __name__ == "__main__":
    root = tk.Tk()
    gui = AudioVisualizerGUI(root)
    root.mainloop()
