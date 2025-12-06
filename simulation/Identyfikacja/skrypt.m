% 1. Wczytanie danych z pliku (separator średnik)
data = readtable('pomiary_temperatury.csv', 'Delimiter', ';', 'VariableNamingRule', 'preserve');

% 2. Przypisanie kolumn do zmiennych (zwróć uwagę na 'data.' przed nazwą)
time_ms = data.Czas_ms;
temperature = data.Temperatura_C;

% 3. Konwersja czasu na sekundy
time_sec = time_ms / 1000;

% 4. Sprawdzenie - powinno wyświetlić np. 0, 0.5, 1.0...
head(time_sec)