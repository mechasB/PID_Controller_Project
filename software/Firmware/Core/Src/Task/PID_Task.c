#include "Task/PID_Task.h"
#include "data.hpp"       
#include "tim.h"          
#include "gpio.h"         
#include "main.h"         
#include <math.h>         

extern osMutexId_t DataMHandle; 
extern SystemData_t g_system_data;     
extern SystemConfig_t g_system_config; 

#define PWM_MAX 1000.0f  
#define PWM_MIN 0.0f

// Histereza wentylatora
#define COOLING_ON_DIFF  2.0f
#define COOLING_OFF_DIFF 0.5f

// Logika READY: 95% wartości zadanej
#define READY_THRESHOLD_FACTOR 0.95f 

static float PID_Calculate(float target, float current, float dt_sec) 
{
    float error = target - current;
    float P = g_system_config.kp * error;
    float integrator = g_system_data.pid_integrator;
    integrator += (g_system_config.ki * error * dt_sec);

    if (integrator > PWM_MAX) integrator = PWM_MAX;
    else if (integrator < PWM_MIN) integrator = PWM_MIN;
    g_system_data.pid_integrator = integrator;

    float output = P + integrator;
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
        g_system_data.is_fan_on = false;   // Reset
        g_system_data.is_ready_on = false; // Reset
        
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

    // --- 1. Odczyt danych ---
    float temp_meas = 0.0f;
    float temp_ref = 0.0f;
    bool fan_status = false; // Lokalna zmienna stanu wentylatora
    
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        temp_meas = g_system_data.measured_value;
        temp_ref = g_system_config.reference_value;
        fan_status = g_system_data.is_fan_on; // Pobieramy aktualny stan, żeby obsłużyć histerezę
        osMutexRelease(DataMHandle);
    }

    // --- 2. Obliczenia PID ---
    float pid_out = 0.0f;
    // (PID liczymy na lokalnych zmiennych, wynik zapiszemy później, żeby nie blokować Mutexa za długo)
    // Ale w tej prostej implementacji zrobimy to w bloku zapisu.

    // --- 3. Logika Wentylatora (Histereza) ---
    float diff_cooling = temp_meas - temp_ref; 
    
    if (diff_cooling >= COOLING_ON_DIFF) {
        fan_status = true; // Włącz
    } 
    else if (diff_cooling <= COOLING_OFF_DIFF) {
        fan_status = false; // Wyłącz
    }
    // W przeciwnym razie (pomiędzy 0.5 a 2.0) zostaje stary stan

    // --- 4. Logika LED READY ---
    // Świeci gdy temp >= 95% celu.
    bool ready_status = false;
    float lower_limit = temp_ref * READY_THRESHOLD_FACTOR;

    if (temp_meas >= lower_limit) {
        ready_status = true;
    } else {
        ready_status = false;
    }
    
    // --- 5. Zapis wyników i sterowanie sprzętem ---
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        pid_out = PID_Calculate(temp_ref, temp_meas, dt_sec);
        
        g_system_data.control_signal = pid_out;
        g_system_data.is_fan_on = fan_status;     // Zapisz stan wentylatora
        g_system_data.is_ready_on = ready_status; // Zapisz stan diody
        
        osMutexRelease(DataMHandle);
    }

    // A. Grzałka (PWM)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)pid_out);

    // B. Wentylator (GPIO)
    HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, fan_status ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // C. Dioda READY (GPIO)
    HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, ready_status ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // D. Dioda HEATING (Opcjonalnie, logiczna indikacja grzania)
    // Świeci jeśli PID pracuje (>0) i jeszcze nie osiągnęliśmy celu
    if ((pid_out > 0.0f) && (temp_meas < temp_ref)) {
        HAL_GPIO_WritePin(HEAT_LED_GPIO_Port, HEAT_LED_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(HEAT_LED_GPIO_Port, HEAT_LED_Pin, GPIO_PIN_RESET);
    }
}