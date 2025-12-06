import serial
import csv
import sys

# --- KONFIGURACJA ---
SERIAL_PORT = '/dev/ttyACM0'   # Ustaw swój port COM (np. COM3, COM4)
BAUD_RATE = 115200     # Musi być taka sama jak w STM32
FILENAME = 'pomiary_temperatury.csv'
# --------------------

def main():
    print(f"Próba połączenia z portem {SERIAL_PORT}...")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Połączono! Dane będą wyświetlane w formacie: Czas;Temperatura;Ciśnienie")
        print("(Naciśnij Ctrl+C, aby zakończyć)")
        print("-" * 40)
        
        # Zmienna do zapamiętania czasu startowego
        first_timestamp = None 

        with open(FILENAME, mode='w', newline='', encoding='utf-8') as csv_file:
            writer = csv.writer(csv_file, delimiter=';')
            
            # Nagłówek w pliku CSV
            writer.writerow(['Czas_ms', 'Temperatura_C', 'Cisnienie_hPa'])

            while True:
                if ser.in_waiting > 0:
                    try:
                        # Odczyt linii i czyszczenie znaków białych
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                    except UnicodeDecodeError:
                        continue

                    if line:
                        data_parts = line.split(';')

                        # Sprawdzamy czy mamy minimum Czas i Temperaturę
                        if len(data_parts) >= 2:
                            try:
                                raw_timestamp = int(data_parts[0])
                            except ValueError:
                                continue 

                            temp = data_parts[1]
                            # Pobieramy ciśnienie tylko jeśli istnieje (zabezpieczenie)
                            pres = data_parts[2] if len(data_parts) > 2 else ""

                            # Ustawienie punktu zerowego przy pierwszym odczycie
                            if first_timestamp is None:
                                first_timestamp = raw_timestamp

                            # Obliczamy czas od zera
                            relative_time = raw_timestamp - first_timestamp

                            if temp and temp.strip():
                                # Zapis do pliku
                                writer.writerow([relative_time, temp, pres])
                                csv_file.flush()
                                
                                # WYŚWIETLANIE W KONSOLI (Format: Czas;Temp;Pres)
                                # Wyświetla się tak jak w terminalu UART, ale z czasem od 0
                                print(f"{relative_time};{temp};{pres}")
                            else:
                                # Opcjonalnie: ignorujemy puste linie lub wypisujemy błąd
                                pass

    except serial.SerialException as e:
        print(f"Błąd portu szeregowego: {e}")
    except KeyboardInterrupt:
        print("\nZakończono logowanie danych.")
        if 'ser' in locals() and ser.is_open:
            ser.close()
        sys.exit(0)

if __name__ == "__main__":
    main()
