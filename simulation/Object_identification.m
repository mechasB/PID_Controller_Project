% ------------------------------------------------------------------------------
%  Opis:        Modelowanie LTI na podstawie pomiarów temperatury (CSV)
% ------------------------------------------------------------------------------

clear; clc; close all;

% --- 1. Wczytanie danych z CSV ------------------------------------------------
filename = 'pomiary_temperatury.csv';
opts = detectImportOptions(filename);
opts.Delimiter = ';'; 
opts.VariableNamingRule = 'preserve';

data = readtable(filename, opts);

% Wyciągnięcie wektorów
temperature_meas = data.Temperatura_C;  % Pomiary (y)
time_ms = data.Czas_ms;                 % Czas w ms

% Konwersja czasu na sekundy (niezbędne do poprawnej stałej czasowej)
t = time_ms / 1000;                     % Czas [s]

% --- 2. Parametry z identyfikacji (cftool) ------------------------------------
% Wpisz tutaj wyniki, które otrzymałeś w cftool:
K_delta_T = 11.10;      % Przyrost temperatury (K z cftool)
T_const   = 210.5;      % Stała czasowa (T z cftool)
Delay_val = 10.9;       % Opóźnienie (theta z cftool)
Temp_init = 22.27;      % Temperatura początkowa (y0 z cftool)

% --- 3. Sygnał wejściowy (wymuszenie) -----------------------------------------
input_val = 90;         % Wartość skoku PWM (90%)
input_signal = input_val * ones(size(t));

% --- 4. Model LTI (linear time-invariant) -------------------------------------
s = tf('s');

% Obliczenie wzmocnienia statycznego obiektu (Kp = zmiana Wyjścia / zmiana Wejścia)
% Kp = 11.10 stopni / 90 procent = 0.1233
k_model = K_delta_T / input_val; 

% Definicja transmitancji: G(s) = k / (T*s + 1) * exp(-T0*s)
H = k_model / (T_const * s + 1);
H.InputDelay = Delay_val; % Ustawienie opóźnienia w obiekcie LTI

disp('Parametry modelu:');
disp(['  Wzmocnienie (Kp): ', num2str(k_model)]);
disp(['  Stała czasowa (T): ', num2str(T_const)]);
disp(['  Opóźnienie: ', num2str(Delay_val)]);

% --- 5. Odpowiedź modelu (Symulacja) ------------------------------------------
% lsim symuluje odpowiedź układu na zadane wejście
model_response_delta = lsim(H, input_signal, t);

% Dodanie temperatury początkowej (offsetu), aby nałożyć wykres na pomiary
model_response_total = model_response_delta + Temp_init;

% --- 6. Błąd modelu -----------------------------------------------------------
residuum = temperature_meas - model_response_total;
error_abs_sum = sum(abs(residuum));
mean_squared_error = mean(residuum.^2);

disp(' ');
disp(sprintf('Suma błędów bezwzględnych (SAE) = %g', error_abs_sum));
disp(sprintf('Błąd średniokwadratowy (MSE) = %g', mean_squared_error));

% --- 7. Wykresy (Wizualizacja) ------------------------------------------------
figure;
subplot(2,1,1);
plot(t, temperature_meas, 'r.', 'MarkerSize', 8); hold on;
plot(t, model_response_total, 'b-', 'LineWidth', 2);
title('Weryfikacja modelu: Pomiary vs Model');
xlabel('Czas [s]'); ylabel('Temperatura [°C]');
legend('Pomiary rzeczywiste', 'Odpowiedź modelu');
grid on;

subplot(2,1,2);
plot(t, residuum, 'k');
title('Residuum (Błąd dopasowania)');
xlabel('Czas [s]'); ylabel('Błąd [°C]');
grid on;