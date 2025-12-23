/**
  ******************************************************************************
  * @file    PID_Task.c
  * @brief   Implements the Control Loop (PID), Fan Hysteresis, and Logic 
  * Indicators (LEDs).
  ******************************************************************************
  */

#include "Task/PID_Task.h"   

extern osMutexId_t DataMHandle; 
extern SystemData_t g_system_data;     
extern SystemConfig_t g_system_config; 

#define PWM_MAX 1000.0f  
#define PWM_MIN 0.0f

/* --- Fan Control Constants --- */
#define COOLING_ON_DIFF  2.0f  ///< Temperature diff to turn FAN ON
#define COOLING_OFF_DIFF 0.5f  ///< Temperature diff to turn FAN OFF

/* --- Ready LED Configuration --- */
/** Tolerance window for Ready LED (+/- 5%) */
#define READY_TOLERANCE_PERCENT 0.05f 

/**
 * @brief  Calculates the PID control output.
 * @param  target   Setpoint temperature.
 * @param  current  Measured temperature.
 * @param  dt_sec   Time delta in seconds since last call.
 * @return Control signal (0.0 to PWM_MAX).
 */
static float PID_Calculate(float target, float current, float dt_sec) 
{
    float error = target - current;
    
    // Proportional term
    float P = g_system_config.kp * error;
    
    // Integral term with Anti-Windup (Clamping)
    float integrator = g_system_data.pid_integrator;
    integrator += (g_system_config.ki * error * dt_sec);

    if (integrator > PWM_MAX) integrator = PWM_MAX;
    else if (integrator < PWM_MIN) integrator = PWM_MIN;
    g_system_data.pid_integrator = integrator;

    // Sum and Final Saturation
    float output = P + integrator;
    if (output > PWM_MAX) output = PWM_MAX;
    else if (output < PWM_MIN) output = PWM_MIN;
    
    g_system_data.pid_error = error;
    return output;
}

/**
 * @brief  Initializes the PID task and PWM hardware.
 */
void PID_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    
    if (osMutexAcquire(DataMHandle, 100) == osOK) {
        // Safe defaults
        g_system_data.pid_integrator = 0.0f;
        g_system_data.control_signal = 0.0f;
        g_system_data.is_fan_on = false;   
        g_system_data.is_ready_on = false; 
        
        g_system_config.kp = 59.20f;
        g_system_config.ki = 0.34f;
        g_system_config.kd = 0.0f; 
        osMutexRelease(DataMHandle);
    }
}

/**
 * @brief  Main update loop for Control Logic.
 * Executes PID, Fan Hysteresis, Safety Interlock, and LED Logic.
 */
void PID_Update(void)
{
    static uint32_t last_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    if (last_tick == 0) {
        last_tick = current_tick;
        return;
    }
    
    // Calculate precise delta time
    float dt_sec = (float)(current_tick - last_tick) / 1000.0f;
    last_tick = current_tick;

    // --- 1. Read Data ---
    float temp_meas = 0.0f;
    float temp_ref = 0.0f;
    bool fan_status = false; 
    
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        temp_meas = g_system_data.measured_value;
        temp_ref = g_system_config.reference_value;
        fan_status = g_system_data.is_fan_on; 
        osMutexRelease(DataMHandle);
    }

    // --- 2. Fan Logic (Hysteresis) ---
    float diff_cooling = temp_meas - temp_ref; 
    
    if (diff_cooling >= COOLING_ON_DIFF) {
        fan_status = true; // Temperature too high -> Fan ON
    } 
    else if (diff_cooling <= COOLING_OFF_DIFF) {
        fan_status = false; // Temperature stabilized -> Fan OFF
    }

    // --- 3. PID Calculation ---
    float pid_out = 0.0f;
    // (Calculation happens below to apply interlock immediately)

    // --- 4. Ready LED Logic (Window Mode +/- 5%) ---
    bool ready_status = false;
    
    // Calculate absolute margin based on setpoint
    float margin = temp_ref * READY_TOLERANCE_PERCENT;
    
    // Check if current temp is within [Target - Margin, Target + Margin]
    if (fabsf(temp_meas - temp_ref) <= margin) {
        ready_status = true;
    } else {
        ready_status = false;
    }
    
    // --- 5. Save Results & Hardware Actuation ---
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        // A. Calculate PID
        pid_out = PID_Calculate(temp_ref, temp_meas, dt_sec);
        
        // B. SAFETY INTERLOCK: If Fan is ON, force PWM to 0 (No heating while cooling)
        if (fan_status == true) {
            pid_out = 0.0f; 
        }
        
        g_system_data.control_signal = pid_out;
        g_system_data.is_fan_on = fan_status;    
        g_system_data.is_ready_on = ready_status; 
        
        osMutexRelease(DataMHandle);
    }

    // --- 6. Hardware Output Control ---

    // A. Heater (PWM)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)pid_out);

    // B. Fan (GPIO)
    HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, fan_status ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // C. Ready LED (GPIO)
    HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, ready_status ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // D. Cooling LED (GPIO) - Matches Fan state
    HAL_GPIO_WritePin(COOLING_LED_GPIO_Port, COOLING_LED_Pin, fan_status ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // E. Heating LED (GPIO) - Logic indication (PWM > 0)
    if (pid_out > 0.0f) {
        HAL_GPIO_WritePin(HEAT_LED_GPIO_Port, HEAT_LED_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(HEAT_LED_GPIO_Port, HEAT_LED_Pin, GPIO_PIN_RESET);
    }
}