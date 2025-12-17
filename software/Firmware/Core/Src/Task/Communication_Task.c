#include "Task/Communication_Task.h"

/* Zmienne zewnętrzne */
extern osMutexId_t DataMHandle;
extern UART_HandleTypeDef huart2;

/* Zmienne do odbioru danych (Rx) */
#define RX_BUFFER_SIZE 64
uint8_t rx_byte;
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_index = 0;

/* --- Prototypy funkcji lokalnych --- */
void Parse_Received_Command(char* cmd);

/* --- Funkcja Inicjalizująca --- */
void Communication_Init(void)
{
    // Uruchomienie odbioru pierwszego bajtu w trybie przerwań
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

/* --- Główna pętla zadania (Update) --- */
void Communication_Update(void)
{
    char tx_buffer[128]; // Zwiększony bufor dla JSON
    
    // Lokalne kopie zmiennych (dla spójności danych)
    float meas_val = 0.0f;
    float ref_val = 0.0f;
    float ctrl_val = 0.0f;
    float pid_error = 0.0f;
    uint32_t timestamp = 0;

    /* 1. Pobranie danych w sekcji krytycznej */
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        meas_val    = g_system_data.measured_value;
        ctrl_val    = g_system_data.control_signal;
        pid_error   = g_system_data.pid_error;
        timestamp   = HAL_GetTick(); // Lub g_system_data.timestamp
        
        ref_val     = g_system_config.reference_value;
        
        osMutexRelease(DataMHandle);
    }

    /* 2. Formatowanie JSON */
    // Format: {"ts":1234, "sp":30.00, "pv":22.50, "cv":500.00, "err":-7.50}
    // ts: timestamp, sp: setpoint, pv: process variable, cv: control value
    int len = snprintf(tx_buffer, sizeof(tx_buffer), 
             "{\"ts\":%lu,\"sp\":%.2f,\"pv\":%.2f,\"cv\":%.2f,\"err\":%.2f}\r\n",
             timestamp, ref_val, meas_val, ctrl_val, pid_error);

    /* 3. Wysyłka UART (DMA lub Blocking - tu Blocking dla uproszczenia w FreeRTOS) */
    if (len > 0 && len < sizeof(tx_buffer))
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, 100);
    }
}

/* --- Obsługa Odbioru (Parsowanie PID) --- */
/* Tę funkcję wołamy, gdy odbierzemy pełną linię (np. w callbacku Rx) */
void Parse_Received_Command(char* cmd)
{
    // Oczekiwany format z Pythona: "PID:Kp:Ki:Kd" np. "PID:2.5:0.1:0.0"
    float new_kp, new_ki, new_kd;

    if (sscanf(cmd, "PID:%f:%f:%f", &new_kp, &new_ki, &new_kd) == 3)
    {
        // Aktualizacja konfiguracji w Mutexie
        if (osMutexAcquire(DataMHandle, 100) == osOK)
        {
            g_system_config.kp = new_kp;
            g_system_config.ki = new_ki;
            g_system_config.kd = new_kd;
            
            // Opcjonalnie: Reset członu całkującego przy zmianie nastaw
            // g_system_data.pid_integrator = 0.0f; 
            
            osMutexRelease(DataMHandle);
        }
    }
}

/* --- Callback Przerwania UART Rx --- */
/* Wklej to tutaj lub w main.c (jeśli nie koliduje) */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_byte == '\n' || rx_byte == '\r') // Koniec linii
        {
            rx_buffer[rx_index] = '\0'; // Zakończ string
            if (rx_index > 0)
            {
                Parse_Received_Command((char*)rx_buffer);
            }
            rx_index = 0; // Reset bufora
        }
        else
        {
            if (rx_index < RX_BUFFER_SIZE - 1)
            {
                rx_buffer[rx_index++] = rx_byte;
            }
        }
        // Wznowienie nasłuchu
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}