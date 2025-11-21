#include "data.h"
#include "semphr.h"

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

// Inicjalizacja mutexa
//SemaphoreHandle_t g_data_mutex = NULL; 

/**
 * @brief Inicjalizuje struktury danych i semafory.
 */
void DATA_Init(void) {
    g_DataSemaphore = xSemaphoreCreateMutex();
}