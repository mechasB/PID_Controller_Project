#include "data.hpp"

// Inicjalizacja zmiennych globalnych
SystemData_t g_system_data = {
    .measured_value = 0.0f,
    .control_signal = 0.0f,
    .pid_error = 0.0f,
    .timestamp = 0,
    .system_status = 0
};

SystemConfig_t g_system_config = {
    .reference_value = 25.0f, // Przykładowa wartość zadana
    .kp = 1.0f,
    .ki = 0.1f,
    .kd = 0.01f,
    .sample_period = 10 // Przykładowy okres próbkowania w ms
};

SystemInterfaceConfig_t g_system_interface_config = {
    .encoder_rotate_value = 0,
    .encoder_btn_status = false,
    .btn_cs_state = false,
    .led_cooling_status = false,
    .led_heat_status = false,
    .led_ready_status = false
};

// Inicjalizacja mutexa
//SemaphoreHandle_t g_data_mutex = NULL; 

/**
 * @brief Inicjalizuje struktury danych i semafory.
 */
void DATA_Init(void) {
    g_DataSemaphore = xSemaphoreCreateMutex();
}