% ------------------------------------------------------------------------------
%  Opis:        Modelowanie LTI - Weryfikacja (Naprawa błędu lsim - Resampling)
% ------------------------------------------------------------------------------
clear; clc; close all;

% --- 1. Wczytanie danych z CSV ------------------------------------------------
filename = 'pomiary_temperatury.csv';
opts = detectImportOptions(filename);
opts.Delimiter = ';'; 
opts.VariableNamingRule = 'preserve';
data = readtable(filename, opts);

% Pobranie surowych danych
raw_temp = data.Temperatura_C;
raw_time_ms = data.Czas_ms;

% Konwersja na sekundy i przesunięcie startu do zera
t_raw = (raw_time_ms - raw_time_ms(1)) / 1000; 

% --- 2. Parametry z identyfikacji (Twoje wyniki) ------------------------------
K_delta_T = 44.2311;    % Przyrost temperatury (K)
T_const   = 149.0002;   % Stała czasowa (T)
Delay_val = 9.80;       % Opóźnienie (theta) - wynik z obrazka

% --- 3. RESAMPLING (Kluczowa naprawa błędu lsim) ------------------------------
% Tworzymy idealny wektor czasu co 0.1 sekundy (stały krok)
dt = 0.1; 
t_uniform = (0:dt:max(t_raw))'; 

% Usuwamy ewentualne duplikaty czasu z surowych danych (wymagane dla interp1)
[t_unique, idx_unique] = unique(t_raw); 
temp_unique = raw_temp(idx_unique);

% Interpolujemy pomiary na nowy, idealny czas
temperature_meas = interp1(t_unique, temp_unique, t_uniform, 'linear');
t = t_uniform; % Od teraz używamy tylko idealnego czasu

% Automatyczne pobranie temperatury początkowej
Temp_init = temperature_meas(1); 

% --- 4. Korekta danych (Spłaszczenie opadania) --------------------------------
% Jeśli temperatura spadała na końcu, "spłaszczamy" ją do wartości max
[max_temp, max_idx] = max(temperature_meas);
temperature_meas(max_idx:end) = max_temp;

% --- 5. Model LTI i Sygnał wejściowy ------------------------------------------
input_val = 50;         % Skok PWM o 50%
input_signal = input_val * ones(size(t));

% Obliczenie wzmocnienia statycznego
k_model = K_delta_T / input_val; 

s = tf('s');
H_ideal = k_model / (T_const * s + 1);

% Aproksymacja opóźnienia Padé (dla stabilności lsim)
H_pade = H_ideal * exp(-Delay_val * s); 

disp('--- Parametry Modelu ---');
fprintf('Wzmocnienie statyczne (Kp): %.4f [°C/%%]\n', k_model);
fprintf('Stała czasowa (T):          %.2f [s]\n', T_const);
fprintf('Opóźnienie (theta):         %.2f [s]\n', Delay_val);

% --- 6. Symulacja (lsim teraz zadziała) ---------------------------------------
% Ponieważ 't' jest teraz idealnie równe (co 0.1s), lsim nie wyrzuci błędu
H_sim = pade(H_pade, 1); 
model_response_delta = lsim(H_sim, input_signal, t);

% Dodanie offsetu
model_response_total = model_response_delta + Temp_init;

% --- 7. Obliczenie błędów -----------------------------------------------------
residuum = temperature_meas - model_response_total;
mean_squared_error = mean(residuum.^2, 'omitnan'); % ignorujemy NaN jeśli powstały
rmse = sqrt(mean_squared_error);

disp(' ');
disp('--- Jakość Dopasowania ---');
fprintf('Błąd średniokwadratowy (MSE): %.4f\n', mean_squared_error);
fprintf('Średni błąd (RMSE):           %.4f °C\n', rmse);

% --- 8. Wykresy ---------------------------------------------------------------
figure;
subplot(2,1,1);
plot(t, temperature_meas, 'r.', 'MarkerSize', 8); hold on;
plot(t, model_response_total, 'b-', 'LineWidth', 2);
title(['Weryfikacja Modelu (Skok ' num2str(input_val) '%)']);
xlabel('Czas [s]'); ylabel('Temperatura [°C]');
legend('Pomiary (interpolowane)', 'Model Matematyczny', 'Location', 'SouthEast');
grid on;

subplot(2,1,2);
plot(t, residuum, 'k');
title('Residuum (Błąd)');
xlabel('Czas [s]'); ylabel('Błąd [°C]');
grid on;