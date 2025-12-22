#ifndef DATA_HPP_
#define DATA_HPP_

#include "main.h"         // Dołącza definicje typów HAL
#include "cmsis_os.h"     // Dołącza definicje FreeRTOS (np. osMutexId_t)
#include <stdbool.h>
#include <stdint.h>

/* --- 1. DEFINICJE STRUKTUR --- */

// Struktura danych dynamicznych (zmieniają się w trakcie pracy)
typedef struct
{
    float measured_value;   // Aktualna temperatura (z BMP280)
    float control_signal;   // Wyliczone sterowanie PID (0.0 - 1000.0)
    
    // Zmienne wewnętrzne regulatora PID (State variables)
    float pid_error;        // Aktualny uchyb (SetPoint - Measured)
    float pid_integrator;   // Suma błędów (człon całkujący) - pamięć
    float pid_prev_error;   // Poprzedni błąd (dla członu różniczkującego)
    
} SystemData_t;

// Struktura konfiguracji (Nastawy)
typedef struct
{
    float reference_value;  // Temperatura zadana (SetPoint)
    
    // Współczynniki PID (ustawiane przez UART lub na sztywno)
    float kp;
    float ki;
    float kd;
    
} SystemConfig_t;

// Struktura interfejsu (Enkoder/Przyciski)
typedef struct
{
    int32_t encoder_rotate_value; // Licznik impulsów enkodera
    bool encoder_btn_status;      // Flaga wciśnięcia przycisku
    
} SystemInterfaceConfig_t;


/* --- 2. DEKLARACJE ZMIENNYCH GLOBALNYCH (EXTERN) --- */
// Dzięki temu inne pliki (.c) widzą te zmienne, ale ich nie tworzą od nowa.

#ifdef __cplusplus
extern "C" {
#endif

// Zmienne z danymi
extern SystemData_t g_system_data;
extern SystemConfig_t g_system_config;
extern SystemInterfaceConfig_t g_system_interface_config;

// Uchwyt do Mutexu (zdefiniowany w main.c lub freertos.c przez CubeMX)
extern osMutexId_t DataMHandle;

#ifdef __cplusplus
}
#endif

#endif /* DATA_HPP_ */