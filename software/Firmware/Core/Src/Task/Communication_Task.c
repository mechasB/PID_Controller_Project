#include "Task/Communication_Task.h"


extern osMutexId_t DataMHandle;
extern UART_HandleTypeDef huart2; // Upewnij się, że to UART od USB

void Communication_Init(void)
{
    // Tu można wysłać powitanie na start
    // char msg[] = "System Start\r\n";
    // HAL_UART_Transmit(&huart3, (uint8_t*)msg, sizeof(msg)-1, 100);
}

void Communication_Update(void)
{
    char tx_buffer[64];
    uint32_t val_to_send = 0;

    /* --- KROK 1: Pobranie danych (Sekcja Krytyczna) --- */
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        // Pobieramy wartość enkodera ze struktury globalnej
        val_to_send = g_system_interface_config.encoder_rotate_value;
        osMutexRelease(DataMHandle);
    }

    /* --- KROK 2: Formatowanie JSON --- */
    // %lu = unsigned long (uint32_t)
    int len = snprintf(tx_buffer, sizeof(tx_buffer), "{\"encoder\":%lu}\r\n", val_to_send);

    /* --- KROK 3: Wysyłka UART --- */
    if (len > 0)
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, 100);
    }
}