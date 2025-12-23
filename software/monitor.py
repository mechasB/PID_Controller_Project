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
    """
    GUI Application for monitoring and tuning the STM32 PID Controller.
    Features: Live Plotting, Serial Communication, Status Indicators.
    """
    def __init__(self, root):
        self.root = root
        self.root.title("PID Monitor")
        self.root.geometry("1100x850")

        self.ser = None
        self.is_reading = False
        
        # Plot Data Buffers (Circular)
        self.max_samples = 300
        self.data_measured = deque(maxlen=self.max_samples)
        self.data_setpoint = deque(maxlen=self.max_samples)
        self.data_error = deque(maxlen=self.max_samples)
        
        # Status Variables
        self.live_pwm = 0.0
        self.live_kp = 0.0; self.live_ki = 0.0; self.live_kd = 0.0
        self.fan_status = 0
        self.rdy_status = 0
        
        # Statistics
        self.crc_errors = 0
        self.sum_squared_error = 0.0
        self.sample_count = 0

        self._setup_ui()
        self._setup_plots()
        self.root.after(100, self.update_gui)

    def _setup_ui(self):
        """Initializes the User Interface layout."""
        control = ttk.LabelFrame(self.root, text="Panel Sterowania", padding=10)
        control.pack(side=tk.TOP, fill=tk.X, padx=10, pady=5)

        # 1. Connection Panel
        conn = ttk.Frame(control)
        conn.pack(side=tk.LEFT, padx=10)
        self.port_combo = ttk.Combobox(conn, width=15)
        self.port_combo.pack(side=tk.LEFT, padx=5)
        ttk.Button(conn, text="⟳", width=3, command=self.refresh_ports).pack(side=tk.LEFT)
        self.btn_connect = ttk.Button(conn, text="Połącz", command=self.toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=5)

        # 2. PID Tuning Panel
        pid_fr = ttk.Frame(control)
        pid_fr.pack(side=tk.LEFT, padx=20)
        self.entries = {}
        for l in ['Kp', 'Ki', 'Kd']:
            ttk.Label(pid_fr, text=f"{l}:").pack(side=tk.LEFT, padx=2)
            e = ttk.Entry(pid_fr, width=6)
            e.pack(side=tk.LEFT, padx=2); e.insert(0, "0.0")
            self.entries[l] = e
        
        # Send Button
        ttk.Button(pid_fr, text="Wyślij Nastawy", command=self.send_pid).pack(side=tk.LEFT, padx=10)
        
        # 3. Reset Stats
        ttk.Button(control, text="Reset Statystyk", command=self.reset_stats).pack(side=tk.RIGHT, padx=10)

        # 4. Status Panel (LEDs & Telemetry)
        status_frame = ttk.LabelFrame(self.root, text="Status Systemu", padding=10)
        status_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=2)
        row1 = ttk.Frame(status_frame); row1.pack(fill=tk.X, pady=5)

        # Visual Indicators
        self.lbl_fan_ind = tk.Label(row1, text=" FAN ", bg="gray", fg="white", font=("Arial", 10, "bold"), width=8)
        self.lbl_fan_ind.pack(side=tk.LEFT, padx=10)

        self.lbl_rdy_ind = tk.Label(row1, text=" READY ", bg="gray", fg="white", font=("Arial", 10, "bold"), width=8)
        self.lbl_rdy_ind.pack(side=tk.LEFT, padx=10)

        self.lbl_pwm = ttk.Label(row1, text="PWM: 0.0%", font=("Arial", 11, "bold"))
        self.lbl_pwm.pack(side=tk.LEFT, padx=20)
        self.lbl_err_pct = ttk.Label(row1, text="Błąd: 0.0%", foreground="red")
        self.lbl_err_pct.pack(side=tk.LEFT, padx=10)

        self.lbl_pid_read = ttk.Label(row1, text="PID: -")
        self.lbl_pid_read.pack(side=tk.RIGHT, padx=10)
        self.lbl_crc = ttk.Label(row1, text="CRC Err: 0", foreground="orange")
        self.lbl_crc.pack(side=tk.RIGHT, padx=10)

        self.refresh_ports()

    def _setup_plots(self):
        """Initializes Matplotlib figures."""
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
        """Opens or closes the Serial Connection."""
        if self.ser and self.ser.is_open:
            self.is_reading = False; self.ser.close(); self.btn_connect.config(text="Połącz")
            self.lbl_fan_ind.config(bg="gray"); self.lbl_rdy_ind.config(bg="gray")
        else:
            try:
                self.ser = serial.Serial(self.port_combo.get(), 115200, timeout=1)
                self.is_reading = True
                threading.Thread(target=self.read_loop, daemon=True).start()
                self.btn_connect.config(text="Rozłącz")
                self.reset_stats()
            except Exception as e: messagebox.showerror("Błąd", str(e))

    def send_pid(self):
        """Formats and sends PID coefficients to STM32."""
        if self.ser and self.ser.is_open:
            try:
                kp = float(self.entries['Kp'].get())
                ki = float(self.entries['Ki'].get())
                kd = float(self.entries['Kd'].get())
                
                # Command format: PID:Kp:Ki:Kd\n
                # \n is crucial for the STM32 interrupt receiver!
                cmd = f"PID:{kp:.2f}:{ki:.4f}:{kd:.2f}\n"
                
                self.ser.write(cmd.encode('utf-8'))
                print(f"[PC -> STM32] Wysłano: {cmd.strip()}")
                
            except ValueError:
                messagebox.showerror("Błąd", "Wprowadź poprawne liczby (użyj kropki jako separatora).")
            except Exception as e:
                messagebox.showerror("Błąd komunikacji", str(e))
        else:
            messagebox.showwarning("Błąd", "Najpierw połącz się z portem COM!")

    def calc_crc8(self, s):
        """Calculates CRC-8 checksum for data validation."""
        crc = 0
        for ch in s:
            crc ^= ord(ch)
            for _ in range(8):
                if crc & 0x80: crc = (crc << 1) ^ 0x07
                else: crc <<= 1
                crc &= 0xFF
        return crc

    def read_loop(self):
        """Background thread for reading Serial data."""
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
                
                self.fan_status = int(d.get("fan", 0))
                self.rdy_status = int(d.get("rdy", 0))

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
        """Updates GUI elements (Labels, Colors, Plots) periodically."""
        if self.is_reading:
            if self.sample_count > 0:
                last_sp = self.data_setpoint[-1]
                pct = (self.data_error[-1]/last_sp*100) if last_sp!=0 else 0
                self.lbl_err_pct.config(text=f"Błąd: {pct:.1f}%")

            self.lbl_pwm.config(text=f"PWM: {self.live_pwm:.1f}%")
            self.lbl_crc.config(text=f"CRC Err: {self.crc_errors}")
            self.lbl_pid_read.config(text=f"Kp: {self.live_kp:.2f} | Ki: {self.live_ki:.3f} | Kd: {self.live_kd:.2f}")

            # Color Update logic
            if self.fan_status == 1: self.lbl_fan_ind.config(bg="#3399FF", text="FAN: ON")
            else: self.lbl_fan_ind.config(bg="#DDDDDD", text="FAN: OFF")

            if self.rdy_status == 1: self.lbl_rdy_ind.config(bg="#00CC00", text="READY")
            else: self.lbl_rdy_ind.config(bg="#DDDDDD", text="WAIT...")

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
