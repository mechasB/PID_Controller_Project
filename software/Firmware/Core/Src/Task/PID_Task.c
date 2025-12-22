#include "Task/PID_Task.h"
#include "data.hpp"       
#include "tim.h"          
#include "gpio.h"         
#include "main.h"         
#include <math.h>         

/* Zmienne zewnętrzne */
extern osMutexId_t DataMHandle; 
extern SystemData_t g_system_data;     
extern SystemConfig_t g_system_config; 

/* Stałe */
#define PWM_MAX 1000.0f  
#define PWM_MIN 0.0f

// Histereza wentylatora
#define COOLING_ON_DIFF  2.0f
#define COOLING_OFF_DIFF 0.5f

// Konfiguracja LED
#define HEATING_TOLERANCE  0.5f 
#define READY_THRESHOLD_FACTOR 0.95f // 95% temperatury zadanej

static float PID_Calculate(float target, float current, float dt_sec) 
{
    float error = target - current;
    
    // P (Proporcjonalny)
    float P = g_system_config.kp * error;

    // I (Całkujący)
    float integrator = g_system_data.pid_integrator;
    integrator += (g_system_config.ki * error * dt_sec);

    // Anti-Windup
    if (integrator > PWM_MAX) integrator = PWM_MAX;
    else if (integrator < PWM_MIN) integrator = PWM_MIN;
    g_system_data.pid_integrator = integrator;

    // Suma + D (jeśli dodasz w przyszłości)
    float output = P + integrator;

    // Saturacja wyjścia
    if (output > PWM_MAX) output = PWM_MAX;
    else if (output < PWM_MIN) output = PWM_MIN;
    
    g_system_data.pid_error = error;
    return output;
}

void PID_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    
    if (osMutexAcquire(DataMHandle, 100) == osOK) {
        g_system_data.pid_integrator = 0.0f;
        g_system_data.control_signal = 0.0f;
        // Domyślne nastawy (bezpieczne)
        g_system_config.kp = 59.20f;
        g_system_config.ki = 0.34f;
        g_system_config.kd = 0.0f; 
        osMutexRelease(DataMHandle);
    }
}

void PID_Update(void)
{
    static uint32_t last_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    if (last_tick == 0) {
        last_tick = current_tick;
        return;
    }
    
    float dt_sec = (float)(current_tick - last_tick) / 1000.0f;
    last_tick = current_tick;

    // 1. Pobranie danych
    float temp_meas = 0.0f;
    float temp_ref = 0.0f;
    
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        temp_meas = g_system_data.measured_value;
        temp_ref = g_system_config.reference_value;
        osMutexRelease(DataMHandle);
    }

    // 2. Obliczenia PID
    float pid_out = 0.0f;
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        pid_out = PID_Calculate(temp_ref, temp_meas, dt_sec);
        g_system_data.control_signal = pid_out;
        osMutexRelease(DataMHandle);
    }

    // 3. Sterowanie sprzętem
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)pid_out);

    // Wentylator (Histereza)
    float diff_cooling = temp_meas - temp_ref; 
    if (diff_cooling >= COOLING_ON_DIFF) {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
    } 
    else if (diff_cooling <= COOLING_OFF_DIFF) {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
    }

    // 4. Logika LED
    // READY (Zielona): Świeci, gdy temp >= 95% zadanej
    float ready_threshold_val = temp_ref * READY_THRESHOLD_FACTOR;
    if (temp_meas >= ready_threshold_val) {
        HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, GPIO_PIN_RESET);
    }

    // HEATING (Czerwona): Świeci, gdy trzeba grzać i PID pracuje
    if ((temp_meas < (temp_ref - HEATING_TOLERANCE)) && (pid_out > 0.0f)) {
        HAL_GPIO_WritePin(HEAT_LED_GPIO_Port, HEAT_LED_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(HEAT_LED_GPIO_Port, HEAT_LED_Pin, GPIO_PIN_RESET);
    }
}