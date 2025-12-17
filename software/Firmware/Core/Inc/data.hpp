
#ifndef _DATA_H
#define _DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
// #include "semphr.h"
// #include "FreeRTOS.h"

#define JSON_TX_BUFFER_SIZE 256

typedef struct {
    float measured_value;
    float control_signal; // Wyjście PID (0-1000 lub 0-100%)
    float pid_error;
    
    float pid_integrator; // Pamięć członu całkującego
    float pid_prev_error; // Poprzedni błąd (dla członu D)
    // ---------------------
    
    uint32_t timestamp;
    uint8_t system_status;
} SystemData_t;

typedef struct {
    float reference_value; // Temperatura zadana
    
    // Nastawy PID
    float kp;
    float ki;
    float kd; 
    
    float sample_period; // w ms (np. 100)
    
    // --- DODANE LIMITY (opcjonalnie, można też hardcodować w #define) ---
    float output_limit_max; // Np. 1000.0 (ARR timera)
    float output_limit_min; // Np. 0.0
    // --------------------------------------------------------------------
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