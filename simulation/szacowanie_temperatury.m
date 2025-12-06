% ------------------------------------------------------------------------------
%  Opis: Szacowanie temperatury końcowej w zależności od wypełnienia PWM
%        na podstawie zidentyfikowanego modelu inercyjnego.
% ------------------------------------------------------------------------------
clear; clc; close all;

% --- 1. Parametry modelu (z identyfikacji) ------------------------------------
% Wpisz tutaj wartości, które wyznaczyliśmy wcześniej:
Temp_otoczenia = 22.27;   % Temperatura początkowa [°C]
K_obiektu      = 11.10;   % Przyrost temp. przy skoku o 90%
Delta_PWM_test = 90;      % Wielkość skoku PWM w teście [%]

% Obliczenie wzmocnienia statycznego [°C / %PWM]
% Mówi nam: o ile stopni wzrośnie temp. przy wzroście PWM o 1%
K_stat = K_obiektu / Delta_PWM_test; 

fprintf('Wzmocnienie statyczne układu: %.4f °C/%%\n', K_stat);
fprintf('Temperatura otoczenia (bazowa): %.2f °C\n\n', Temp_otoczenia);

% --- 2. Obliczenia dla całego zakresu (0-100%) --------------------------------
PWM_range = 0:1:100; % Wektor wypełnienia od 0 do 100 co 1%

% Wzór liniowy: T_koncowa = T_otoczenia + (K_stat * PWM)
Temp_steady_state = Temp_otoczenia + (K_stat * PWM_range);

% --- 3. Wykres charakterystyki statycznej -------------------------------------
figure;
plot(PWM_range, Temp_steady_state, 'b-', 'LineWidth', 2);
grid on;
title('Charakterystyka statyczna: Temperatura vs PWM');
xlabel('Wypełnienie PWM [%]');
ylabel('Szacowana temperatura ustalona [°C]');
xlim([0 100]);

% Dodanie punktów charakterystycznych na wykresie
hold on;
target_pwms = [10, 50, 90, 100]; % Punkty do zaznaczenia
for pwm_val = target_pwms
    temp_val = Temp_otoczenia + (K_stat * pwm_val);
    plot(pwm_val, temp_val, 'ro', 'MarkerFaceColor', 'r');
    text(pwm_val+2, temp_val, sprintf('%.1f°C', temp_val), 'FontSize', 10);
end

% --- 4. Kalkulator dla użytkownika --------------------------------------------
% Możesz wpisać własną wartość w Command Window po uruchomieniu
disp('--- Przykładowe szacunki ---');
fprintf('Dla PWM = 10%%  -> Temperatura ok. %.2f °C\n', Temp_otoczenia + K_stat*10);
fprintf('Dla PWM = 50%%  -> Temperatura ok. %.2f °C\n', Temp_otoczenia + K_stat*50);
fprintf('Dla PWM = 100%% -> Temperatura ok. %.2f °C\n', Temp_otoczenia + K_stat*100);

disp(' ');
user_pwm = input('Podaj własną wartość PWM (0-100), aby obliczyć temperaturę: ');

if ~isempty(user_pwm)
    user_temp = Temp_otoczenia + (K_stat * user_pwm);
    fprintf('-> Przy PWM %.1f%% układ nagrzeje się do ok. %.2f °C\n', user_pwm, user_temp);
end