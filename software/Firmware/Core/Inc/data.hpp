
#ifndef _DATA_H
#define _DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
// #include "semphr.h"
// #include "FreeRTOS.h"

#define JSON_TX_BUFFER_SIZE 256

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

typedef struct{
    uint32_t encoder_rotate_value;
    bool encoder_btn_status;
    bool btn_cs_state;
    bool led_cooling_status;
    bool led_heat_status;
    bool led_ready_status; 
} SystemInterfaceConfig_t;

extern SystemData_t g_system_data;
extern SystemConfig_t g_system_config;
extern SystemInterfaceConfig_t g_system_interface_config;

#endif // DATA_H