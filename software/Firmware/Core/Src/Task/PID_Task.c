#include "Task/PID_Task.h"

/* External Variables */
extern osMutexId_t DataMHandle; 
extern SystemData_t g_system_data;     
extern SystemConfig_t g_system_config; 

/* Private Constants */
#define PWM_MAX 1000.0f  // Matches Timer ARR
#define PWM_MIN 0.0f
#define COOLING_ON_DIFF  2.0f
#define COOLING_OFF_DIFF 0.5f

/* --- Private Helper Functions --- */

/**
 * @brief Calculates PID output and updates integrator state.
 */
static float PID_Calculate(float target, float current, float dt_sec) 
{
    float error = target - current;
    
    // 1. Proportional Term
    float P = g_system_config.kp * error;

    // 2. Integral Term
    // Retrieve previous integrator value
    float integrator = g_system_data.pid_integrator;
    
    integrator += (g_system_config.ki * error * dt_sec);

    // Anti-Windup (Clamping)
    if (integrator > PWM_MAX) integrator = PWM_MAX;
    else if (integrator < PWM_MIN) integrator = PWM_MIN;

    // Save updated integrator back to global state
    g_system_data.pid_integrator = integrator;

    // 3. Output Sum
    float output = P + integrator;

    // 4. Output Saturation
    if (output > PWM_MAX) output = PWM_MAX;
    else if (output < PWM_MIN) output = PWM_MIN;
    
    // Store error for debugging
    g_system_data.pid_error = error;

    return output;
}

/* --- Public Functions --- */

void PID_Init(void)
{
    // Start PWM on TIM3 Channel 1
    // MAKE SURE htim3 is initialized in main.c before this task starts!
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    
    // Reset PID state safely
    if (osMutexAcquire(DataMHandle, 100) == osOK) {
        g_system_data.pid_integrator = 0.0f;
        g_system_data.pid_prev_error = 0.0f;
        g_system_data.control_signal = 0.0f;
        osMutexRelease(DataMHandle);
    }
}

void PID_Update(void)
{
    static uint32_t last_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    // Calculate precise dt (delta time) in seconds
    // First run protection
    if (last_tick == 0) {
        last_tick = current_tick;
        return;
    }
    
    float dt_sec = (float)(current_tick - last_tick) / 1000.0f;
    last_tick = current_tick;

    // --- 1. Get Data (Mutex Protected) ---
    float temp_meas = 0.0f;
    float temp_ref = 0.0f;
    
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        temp_meas = g_system_data.measured_value;
        temp_ref = g_system_config.reference_value;
        osMutexRelease(DataMHandle);
    }

    // --- 2. PID Calculation (Mutex Protected for consistency) ---
    float pid_out = 0.0f;
    
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        pid_out = PID_Calculate(temp_ref, temp_meas, dt_sec);
        g_system_data.control_signal = pid_out;
        osMutexRelease(DataMHandle);
    }

    // --- 3. Actuate Hardware ---
    
    // A. Heater (PWM)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)pid_out);

    // B. Fan (Hysteresis Logic)
    float diff = temp_meas - temp_ref;
    
    if (diff >= COOLING_ON_DIFF) {
        // Turn Fan ON
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
    } 
    else if (diff <= COOLING_OFF_DIFF) {
        // Turn Fan OFF
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
    }
}