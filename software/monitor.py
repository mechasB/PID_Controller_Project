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
        self.root.title("PID Controller Monitor & Tuner")
        self.root.geometry("1000x800")

        # --- Zmienne komunikacyjne ---
        self.ser = None
        self.is_reading = False
        self.serial_thread = None
        
        # --- Zmienne danych (bufor kołowy - ostatnie 200 próbek) ---
        self.max_samples = 200
        self.data_time = deque(maxlen=self.max_samples)
        self.data_setpoint = deque(maxlen=self.max_samples)
        self.data_measured = deque(maxlen=self.max_samples)
        self.data_error = deque(maxlen=self.max_samples)
        self.data_control = deque(maxlen=self.max_samples) # Do obliczania kosztu

        # --- Statystyki ---
        self.sum_squared_error = 0.0
        self.sample_count = 0
        self.control_cost = 0.0

        self._setup_ui()
        self._setup_plots()
        
        # Uruchomienie pętli aktualizacji GUI
        self.root.after(100, self.update_gui)

    def _setup_ui(self):
        # Główny panel sterowania
        control_frame = ttk.LabelFrame(self.root, text="Panel Sterowania", padding=10)
        control_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=5)

        # 1. Sekcja połączenia
        conn_frame = ttk.Frame(control_frame)
        conn_frame.pack(side=tk.LEFT, padx=10)
        
        ttk.Label(conn_frame, text="Port COM:").pack(side=tk.LEFT)
        self.port_combo = ttk.Combobox(conn_frame, width=10)
        self.port_combo.pack(side=tk.LEFT, padx=5)
        
        self.btn_refresh = ttk.Button(conn_frame, text="⟳", width=3, command=self.refresh_ports)
        self.btn_refresh.pack(side=tk.LEFT, padx=2)
        
        self.btn_connect = ttk.Button(conn_frame, text="Połącz", command=self.toggle_connection)
        self.btn_connect.pack(side=tk.LEFT, padx=5)

        # 2. Sekcja PID
        pid_frame = ttk.Frame(control_frame)
        pid_frame.pack(side=tk.LEFT, padx=20)
        
        # Pola Kp, Ki, Kd
        self.entries = {}
        for i, label in enumerate(['Kp', 'Ki', 'Kd']):
            ttk.Label(pid_frame, text=f"{label}:").pack(side=tk.LEFT, padx=2)
            entry = ttk.Entry(pid_frame, width=6)
            entry.pack(side=tk.LEFT, padx=2)
            entry.insert(0, "0.0") # Domyślna wartość
            self.entries[label] = entry

        self.btn_send = ttk.Button(pid_frame, text="Wyślij Nastawy", command=self.send_pid)
        self.btn_send.pack(side=tk.LEFT, padx=10)

        # 3. Sekcja Wskaźników Jakości
        stats_frame = ttk.LabelFrame(self.root, text="Wskaźniki Jakości (Real-time)", padding=10)
        stats_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=2)
        
        self.lbl_mse = ttk.Label(stats_frame, text="MSE (Błąd średniokw.): 0.000", font=("Arial", 10, "bold"))
        self.lbl_mse.pack(side=tk.LEFT, padx=20)
        
        self.lbl_cost = ttk.Label(stats_frame, text="Koszt sterowania (Energy): 0.000", font=("Arial", 10))
        self.lbl_cost.pack(side=tk.LEFT, padx=20)
        
        ttk.Button(stats_frame, text="Reset Statystyk", command=self.reset_stats).pack(side=tk.RIGHT)

        # Inicjalizacja listy portów
        self.refresh_ports()

    def _setup_plots(self):
        # Konfiguracja Matplotlib
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(8, 6), sharex=True)
        plt.subplots_adjust(bottom=0.1, hspace=0.3)

        # Wykres 1: Temp Zadana vs Mierzona
        self.line_sp, = self.ax1.plot([], [], 'g--', label='Zadana (SP)')
        self.line_pv, = self.ax1.plot([], [], 'b-', label='Obecna (PV)', linewidth=2)
        self.ax1.set_ylabel("Temperatura [°C]")
        self.ax1.set_title("Regulacja Procesu")
        self.ax1.legend(loc="upper left")
        self.ax1.grid(True)

        # Wykres 2: Uchyb
        self.line_err, = self.ax2.plot([], [], 'r-', label='Uchyb (Error)')
        self.ax2.set_ylabel("Uchyb [°C]")
        self.ax2.set_xlabel("Czas [próbki]")
        self.ax2.axhline(0, color='black', linewidth=1, linestyle='--')
        self.ax2.grid(True)
        self.ax2.legend(loc="upper right")

        # Osadzenie w Tkinter
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        self.port_combo['values'] = [p.device for p in ports]
        if ports:
            self.port_combo.current(0)

    def toggle_connection(self):
        if self.ser and self.ser.is_open:
            # Rozłączanie
            self.is_reading = False
            self.ser.close()
            self.btn_connect.config(text="Połącz")
            self.set_ui_state(tk.NORMAL)
            print("Rozłączono.")
        else:
            # Łączenie
            port = self.port_combo.get()
            try:
                self.ser = serial.Serial(port, 115200, timeout=1)
                self.is_reading = True
                self.serial_thread = threading.Thread(target=self.read_serial_loop, daemon=True)
                self.serial_thread.start()
                
                self.btn_connect.config(text="Rozłącz")
                self.set_ui_state(tk.DISABLED) # Zablokuj zmianę portu w trakcie
                print(f"Połączono z {port}")
                self.reset_stats()
            except Exception as e:
                messagebox.showerror("Błąd", f"Nie można otworzyć portu: {e}")

    def set_ui_state(self, state):
        self.port_combo.config(state=state)
        self.btn_refresh.config(state=state)

    def send_pid(self):
        if self.ser and self.ser.is_open:
            try:
                kp = float(self.entries['Kp'].get())
                ki = float(self.entries['Ki'].get())
                kd = float(self.entries['Kd'].get())
                # Format ramki: "PID:Kp:Ki:Kd\n"
                cmd = f"PID:{kp:.2f}:{ki:.4f}:{kd:.2f}\n"
                self.ser.write(cmd.encode('utf-8'))
                print(f"Wysłano: {cmd.strip()}")
            except ValueError:
                messagebox.showwarning("Błąd", "Wartości PID muszą być liczbami.")
        else:
            messagebox.showwarning("Błąd", "Brak połączenia z urządzeniem.")

    def read_serial_loop(self):
        while self.is_reading and self.ser.is_open:
            try:
                # Odczyt linii i dekodowanie
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                
                # --- TUTAJ JEST ZMIANA NA JSON ---
                try:
                    # Próba parsowania JSON
                    data_json = json.loads(line)
                    
                    # Pobieranie danych po kluczach z JSON-a STM32
                    # Klucze: "ts", "sp", "pv", "cv", "err"
                    t = self.sample_count  # Używamy licznika próbek dla osi X
                    sp = float(data_json.get("sp", 0.0))
                    pv = float(data_json.get("pv", 0.0))
                    ctrl = float(data_json.get("cv", 0.0))
                    err = float(data_json.get("err", 0.0))

                    # Dodawanie do buforów
                    self.data_time.append(t)
                    self.data_setpoint.append(sp)
                    self.data_measured.append(pv)
                    self.data_error.append(err)
                    self.data_control.append(ctrl)

                    # Statystyki
                    self.sum_squared_error += (err ** 2)
                    self.control_cost += (ctrl ** 2) * 0.1 
                    self.sample_count += 1
                    
                except json.JSONDecodeError:
                    # Ignorujemy linie, które nie są JSON-em (np. komunikaty debugowe)
                    print(f"Ignored raw line: {line}")
                    pass

            except Exception as e:
                print(f"Błąd odczytu: {e}")
                break

    def reset_stats(self):
        self.sum_squared_error = 0.0
        self.sample_count = 0
        self.control_cost = 0.0
        self.data_time.clear()
        self.data_setpoint.clear()
        self.data_measured.clear()
        self.data_error.clear()

    def update_gui(self):
        # Ta funkcja jest wywoływana cyklicznie w głównym wątku GUI
        if self.is_reading:
            # 1. Aktualizacja wskaźników tekstowych
            if self.sample_count > 0:
                mse = self.sum_squared_error / self.sample_count
                self.lbl_mse.config(text=f"MSE: {mse:.4f}")
                self.lbl_cost.config(text=f"Koszt (u^2): {self.control_cost:.1f}")

            # 2. Aktualizacja wykresów
            # Tworzymy oś X (indeksy)
            x_data = range(len(self.data_measured))
            
            self.line_sp.set_data(x_data, self.data_setpoint)
            self.line_pv.set_data(x_data, self.data_measured)
            self.line_err.set_data(x_data, self.data_error)

            # Skalowanie osi
            if len(self.data_measured) > 1:
                self.ax1.set_xlim(0, len(self.data_measured))
                self.ax2.set_xlim(0, len(self.data_measured))
                
                self.ax1.relim()
                self.ax1.autoscale_view()
                self.ax2.relim()
                self.ax2.autoscale_view()
            
            self.canvas.draw()
        
        # Zaplanuj kolejne odświeżenie za 100ms
        self.root.after(100, self.update_gui)

if __name__ == "__main__":
    root = tk.Tk()
    # Ustawienie tematu dla lepszego wyglądu (opcjonalne)
    style = ttk.Style()
    style.theme_use('clam') 
    
    app = PIDMonitorApp(root)
    root.mainloop()
