#include "data.hpp"

/* --- INICJALIZACJA ZMIENNYCH GLOBALNYCH --- */

// 1. Dane systemowe (startujemy od zer)
SystemData_t g_system_data = {
    .measured_value = 0.0f,
    .control_signal = 0.0f,
    .pid_error      = 0.0f,
    .pid_integrator = 0.0f,  // Ważne: zerowanie całki przy starcie
    .pid_prev_error = 0.0f,

    .is_fan_on = 0,     // Czy wentylator się kręci?
    .is_ready_on = 0
};

// 2. Konfiguracja (Wartości domyślne PID i temperatury)
SystemConfig_t g_system_config = {
    .reference_value = 25.0f, // Domyślna temperatura zadana
    
    // Domyślne nastawy PID (bezpieczne wartości na start)
    .kp = 59.2041f,
    .ki = 0.34173f,
    .kd = 0.0f        // Domyślnie człon D wyłączony
};

// 3. Interfejs
SystemInterfaceConfig_t g_system_interface_config = {
    .encoder_rotate_value = 0,
    .encoder_btn_status = false
};

// UWAGA: Zmienna 'DataMHandle' nie jest tu definiowana, 
// ponieważ jest generowana przez CubeMX w pliku freertos.c lub main.c.
// W data.hpp mamy tylko 'extern' do niej.