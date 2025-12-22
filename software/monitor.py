import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from collections import deque
import threading
import time
import json

class PIDMonitorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("PID Controller Monitor (CRC + Settings)")
        self.root.geometry("1000x850")

        self.ser = None
        self.is_reading = False
        
        # Dane
        self.max_samples = 200
        self.data_time = deque(maxlen=self.max_samples)
        self.data_setpoint = deque(maxlen=self.max_samples)
        self.data_measured = deque(maxlen=self.max_samples)
        self.data_error = deque(maxlen=self.max_samples)
        
        # Zmienne Live
        self.live_pwm = 0.0
        self.live_kp = 0.0
        self.live_ki = 0.0
        self.live_kd = 0.0
        self.crc_errors = 0 
        self.sum_squared_error = 0.0
        self.sample_count = 0

        self._setup_ui()
        self._setup_plots()
        self.root.after(100, self.update_gui)

    def _setup_ui(self):
        control = ttk.LabelFrame(self.root, text="Panel Sterowania", padding=10)
        control.pack(side=tk.TOP, fill=tk.X, padx=10, pady=5)

        # Połączenie
        conn = ttk.Frame(control)
        conn.pack(side=tk.LEFT, padx=10)
        self.port_combo = ttk.Combobox(conn, width=15)
        self.port_combo.pack(side=tk.LEFT, padx=5)
        ttk.Button(conn, text="⟳", width=3, command=self.refresh_ports).pack(side=tk.LEFT)
        self.btn_connect = ttk.Button(conn, text="Połącz", command=self.toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=5)

        # Nastawy PID
        pid_fr = ttk.Frame(control)
        pid_fr.pack(side=tk.LEFT, padx=20)
        self.entries = {}
        for l in ['Kp', 'Ki', 'Kd']:
            ttk.Label(pid_fr, text=f"{l}:").pack(side=tk.LEFT, padx=2)
            e = ttk.Entry(pid_fr, width=6)
            e.pack(side=tk.LEFT, padx=2); e.insert(0, "0.0")
            self.entries[l] = e
        ttk.Button(pid_fr, text="Wyślij", command=self.send_pid).pack(side=tk.LEFT, padx=10)

        # Status
        status = ttk.LabelFrame(self.root, text="Status", padding=10)
        status.pack(side=tk.TOP, fill=tk.X, padx=10, pady=2)
        row1 = ttk.Frame(status); row1.pack(fill=tk.X)
        
        self.lbl_mse = ttk.Label(row1, text="MSE: 0.000", width=12)
        self.lbl_mse.pack(side=tk.LEFT)
        self.lbl_err_pct = ttk.Label(row1, text="Błąd wzgl.: 0.0%", width=16, foreground="red")
        self.lbl_err_pct.pack(side=tk.LEFT)
        self.lbl_pwm = ttk.Label(row1, text="PWM: 0.0%", font=("Arial", 11, "bold"), foreground="blue")
        self.lbl_pwm.pack(side=tk.LEFT, padx=20)
        self.lbl_crc = ttk.Label(row1, text="CRC Err: 0", foreground="orange")
        self.lbl_crc.pack(side=tk.LEFT, padx=20)

        row2 = ttk.Frame(status); row2.pack(fill=tk.X)
        ttk.Label(row2, text="PID (MCU): ", font=("bold")).pack(side=tk.LEFT)
        self.lbl_pid_read = ttk.Label(row2, text="-")
        self.lbl_pid_read.pack(side=tk.LEFT)

        self.refresh_ports()

    def _setup_plots(self):
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(8, 6), sharex=True)
        plt.subplots_adjust(bottom=0.1, hspace=0.3)
        self.line_sp, = self.ax1.plot([], [], 'g--', label='Zadana')
        self.line_pv, = self.ax1.plot([], [], 'b-', label='Obecna')
        self.ax1.legend(); self.ax1.grid(True)
        self.line_err, = self.ax2.plot([], [], 'r-', label='Uchyb')
        self.ax2.legend(); self.ax2.grid(True)
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)
        self.canvas.get_tk_widget().pack(expand=True, fill=tk.BOTH)

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        p_names = [p.device for p in ports]
        self.port_combo['values'] = p_names
        if p_names:
            best = next((i for i, n in enumerate(p_names) if "ACM" in n or "USB" in n), 0)
            self.port_combo.current(best)

    def toggle_connection(self):
        if self.ser and self.ser.is_open:
            self.is_reading = False; self.ser.close(); self.btn_connect.config(text="Połącz")
        else:
            try:
                self.ser = serial.Serial(self.port_combo.get(), 115200, timeout=1)
                self.is_reading = True
                threading.Thread(target=self.read_loop, daemon=True).start()
                self.btn_connect.config(text="Rozłącz")
                self.reset_stats()
            except Exception as e: messagebox.showerror("Błąd", str(e))

    def send_pid(self):
        if self.ser and self.ser.is_open:
            try:
                cmd = f"PID:{float(self.entries['Kp'].get()):.2f}:{float(self.entries['Ki'].get()):.4f}:{float(self.entries['Kd'].get()):.2f}\n"
                self.ser.write(cmd.encode('utf-8'))
                print(f"Sent: {cmd.strip()}")
            except: pass

    def calc_crc8(self, s):
        crc = 0
        for ch in s:
            crc ^= ord(ch)
            for _ in range(8):
                if crc & 0x80: crc = (crc << 1) ^ 0x07
                else: crc <<= 1
                crc &= 0xFF
        return crc

    def read_loop(self):
        while self.is_reading and self.ser.is_open:
            try:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if not line or '|' not in line: continue
                
                parts = line.split('|')
                if len(parts) != 2: continue
                
                json_str, crc_hex = parts[0], parts[1]
                if self.calc_crc8(json_str) != int(crc_hex, 16):
                    self.crc_errors += 1
                    continue

                d = json.loads(json_str)
                self.live_pwm = float(d.get("pwm", 0))
                self.live_kp = float(d.get("kp", 0))
                self.live_ki = float(d.get("ki", 0))
                self.live_kd = float(d.get("kd", 0))
                
                sp, pv = float(d.get("sp", 0)), float(d.get("pv", 0))
                err = float(d.get("err", 0))

                self.data_measured.append(pv)
                self.data_setpoint.append(sp)
                self.data_error.append(err)
                self.sum_squared_error += err**2
                self.sample_count += 1
            except: pass

    def reset_stats(self):
        self.sum_squared_error = 0; self.sample_count = 0; self.crc_errors = 0
        self.data_measured.clear(); self.data_setpoint.clear(); self.data_error.clear()

    def update_gui(self):
        if self.is_reading:
            if self.sample_count > 0:
                self.lbl_mse.config(text=f"MSE: {self.sum_squared_error/self.sample_count:.4f}")
                last_sp = self.data_setpoint[-1]
                pct = (self.data_error[-1]/last_sp*100) if last_sp!=0 else 0
                self.lbl_err_pct.config(text=f"Błąd wzgl.: {pct:.1f}%")

            self.lbl_pwm.config(text=f"PWM: {self.live_pwm:.1f}%")
            self.lbl_crc.config(text=f"CRC Err: {self.crc_errors}")
            self.lbl_pid_read.config(text=f"Kp: {self.live_kp:.2f} | Ki: {self.live_ki:.3f} | Kd: {self.live_kd:.2f}")

            x = range(len(self.data_measured))
            self.line_sp.set_data(x, self.data_setpoint)
            self.line_pv.set_data(x, self.data_measured)
            self.line_err.set_data(x, self.data_error)
            
            if len(x)>1:
                self.ax1.set_xlim(0, len(x)); self.ax1.relim(); self.ax1.autoscale_view()
                self.ax2.set_xlim(0, len(x)); self.ax2.relim(); self.ax2.autoscale_view()
            self.canvas.draw()
        self.root.after(100, self.update_gui)

if __name__ == "__main__":
    root = tk.Tk()
    style = ttk.Style(); style.theme_use('clam')
    app = PIDMonitorApp(root)
    root.mainloop()
