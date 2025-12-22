#include "Task/Communication_Task.h"
#include "data.hpp"       
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Wymagane dla atof i strtok

/* --- Zmienne zewnętrzne --- */
extern osMutexId_t DataMHandle;
extern UART_HandleTypeDef huart2;

/* --- Funkcja CRC (bez zmian) --- */
uint8_t Calculate_CRC8(const char *data, uint16_t length)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc;
}

void Communication_Init(void)
{
    // Opcjonalne: Wyczyść flagi błędów na starcie
    __HAL_UART_CLEAR_OREFLAG(&huart2);
    __HAL_UART_CLEAR_NEFLAG(&huart2);
    __HAL_UART_CLEAR_FEFLAG(&huart2);
}

void Communication_Update(void)
{
    static uint32_t last_tx_tick = 0;
    static char rx_buffer[64];
    static uint8_t rx_index = 0;
    static char tx_buffer[256]; 

    // --- CZĘŚĆ 0: RATUNEK PRZED BŁĘDEM ORE (Overrun Error) ---
    // To jest kluczowe przy pollngu! Jeśli flaga ORE jest ustawiona, UART nie odbiera.
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE))
    {
        __HAL_UART_CLEAR_OREFLAG(&huart2);
        // Opcjonalnie: mrugnij diodą błędu, jeśli masz
    }

    // --- CZĘŚĆ 1: ODBIÓR DANYCH (Polling) ---
    uint8_t rx_byte;
    
    // Odbieramy tak długo, jak są dane w buforze
    while (HAL_UART_Receive(&huart2, &rx_byte, 1, 0) == HAL_OK)
    {
        // DEBUG: Odsyłamy znak z powrotem (Echo), żebyś widział w terminalu PC czy działa
        // (Możesz zakomentować po testach)
        // HAL_UART_Transmit(&huart2, &rx_byte, 1, 10); 

        // Czy koniec linii? (\n lub \r)
        if (rx_byte == '\n' || rx_byte == '\r')
        {
            rx_buffer[rx_index] = 0; // Zamknij string

            // Parsowanie przy użyciu strtok (bezpieczniejsze)
            // Oczekujemy: "PID:10.5:0.2:1.0"
            if (strncmp(rx_buffer, "PID", 3) == 0)
            {
                // strtok dzieli tekst po separatorach ":"
                char *token = strtok(rx_buffer, ":"); // To będzie "PID"
                
                // Pobieramy kolejne części
                char *str_kp = strtok(NULL, ":");
                char *str_ki = strtok(NULL, ":");
                char *str_kd = strtok(NULL, ":");

                if (str_kp != NULL && str_ki != NULL && str_kd != NULL)
                {
                    // Konwersja na float (atof jest w stdlib.h)
                    float new_kp = (float)atof(str_kp);
                    float new_ki = (float)atof(str_ki);
                    float new_kd = (float)atof(str_kd);

                    // Zapis do zmiennych globalnych
                    if (osMutexAcquire(DataMHandle, 10) == osOK)
                    {
                        g_system_config.kp = new_kp;
                        g_system_config.ki = new_ki;
                        g_system_config.kd = new_kd;
                        osMutexRelease(DataMHandle);
                    }
                }
            }
            rx_index = 0; // Reset bufora
        }
        else
        {
            // Zapisz znak jeśli jest miejsce
            if (rx_index < (sizeof(rx_buffer) - 1))
            {
                rx_buffer[rx_index++] = rx_byte;
            }
            else
            {
                rx_index = 0; // Safety reset
            }
        }
    }

    // --- CZĘŚĆ 2: WYSYŁANIE DANYCH (Co 500ms) ---
    uint32_t current_tick = HAL_GetTick();
    if ((current_tick - last_tx_tick) >= 500)
    {
        last_tx_tick = current_tick;
        
        float sp=0, pv=0, cv=0, err=0, k_p=0, k_i=0, k_d=0, pwm_pct=0;
        
        if (osMutexAcquire(DataMHandle, 10) == osOK) {
            sp = g_system_config.reference_value;
            k_p = g_system_config.kp; k_i = g_system_config.ki; k_d = g_system_config.kd;
            pv = g_system_data.measured_value;
            cv = g_system_data.control_signal;
            err = g_system_data.pid_error;
            osMutexRelease(DataMHandle);
        }
        
        pwm_pct = cv / 10.0f;
        if(pwm_pct > 100) pwm_pct=100; if(pwm_pct < 0) pwm_pct=0;

        int len = snprintf(tx_buffer, sizeof(tx_buffer)-10, 
            "{\"ts\":%lu,\"sp\":%.2f,\"pv\":%.2f,\"cv\":%.2f,\"pwm\":%.1f,\"err\":%.2f,\"kp\":%.2f,\"ki\":%.3f,\"kd\":%.2f}", 
            (unsigned long)current_tick, sp, pv, cv, pwm_pct, err, k_p, k_i, k_d);

        if (len > 0) {
            uint8_t crc = Calculate_CRC8(tx_buffer, len);
            int ft = snprintf(tx_buffer+len, sizeof(tx_buffer)-len, "|%02X\r\n", crc);
            if(ft>0) HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len+ft, 100);
        }
    }
}