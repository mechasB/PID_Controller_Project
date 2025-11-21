
#ifndef _DATA_H
#define _DATA_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "task.h"
#include "semphr.h" 


#define JSON_TX_BUFFER_SIZE 256
extern SemaphoreHandle_t g_DataSemaphore; 


typedef struct {
    float measured_value;   // Aktualna wartość mierzona (element pomiarowy)
    float control_signal;   // Sygnał sterujący (element wykonawczy)
    float pid_error;        // Błąd regulacji (algorytm sterowania)
    uint32_t timestamp;     // Czas od startu systemu (do synchronizacji z PC)
    uint8_t system_status;  // Status pracy (np. 0-OK, 1-Błąd)
} SystemData_t;


typedef struct {
    float reference_value;  // Wartość referencyjna/zadana (interfejs użytkownika)
    float kp, ki, kd;       // Nastawy regulatora PID (modyfikacja parametrów )
    uint16_t sample_period; // Okres próbkowania
} SystemConfig_t;


//extern SystemData_t g_system_data;
//extern SystemConfig_t g_system_config;
//extern SemaphoreHandle_t g_data_mutex; 

#endif // DATA_H