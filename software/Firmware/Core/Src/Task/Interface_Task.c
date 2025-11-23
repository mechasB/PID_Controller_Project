/**
  ******************************************************************************
  * @file    Interface_Task.cpp
  * @brief   Implementation of encoder reading logic.
  ******************************************************************************
  */

#include "Task/Interface_Task.h"

/* Private Includes */
/* External Variables */
// Access to the Mutex defined in freertos.c/main.c
extern osMutexId_t DataMHandle; 

/* --- Private Variables --- */
static uint32_t enc_last_counter = 0;
static bool btn_last_state = true; // True = Released (Pull-Up)

/* --- Helper Functions --- */

/**
 * @brief  Calculates logic step from raw encoder ticks.
 * Handles 16-bit overflow/underflow (0 <-> 65535).
 * @return int8_t: +1 (Right), -1 (Left), 0 (None)
 */
static int8_t Encoder_Get_Step(void)
{
    // Read raw value from library (returns uint32_t)
    uint32_t curr_counter = ENC_ReadCounter(&henc1);
    
    // Calculate difference dealing with 16-bit wrap-around
    // Casting to uint16_t truncates data, casting to int16_t makes it signed
    int16_t diff = (int16_t)((uint16_t)curr_counter - (uint16_t)enc_last_counter);

    // Threshold: 2 ticks = 1 physical step
    if (diff >= 2 || diff <= -2)
    {
        enc_last_counter = curr_counter;
        return (int8_t)(diff / 2);
    }
    return 0;
}

/**
 * @brief  Checks for button press (Falling Edge).
 * @return bool: true if just pressed.
 */
static bool Button_Is_Pressed(void)
{
    // Read pin: 0 = Pressed, 1 = Released
    bool curr_state = (HAL_GPIO_ReadPin(ENCODER_BTN_GPIO_Port, ENCODER_BTN_Pin) == GPIO_PIN_SET);
    bool pressed = false;

    // Detect transition High -> Low
    if (btn_last_state == true && curr_state == false)
    {
        pressed = true;
    }
    btn_last_state = curr_state;
    return pressed;
}

/* --- Public Functions --- */

void Interface_Init(void)
{
    // Initialize Encoder Library
    ENC_Init(&henc1);
    
    // Reset hardware counter to 0
    // Using direct register access or library function if available
    __HAL_TIM_SET_COUNTER(henc1.Timer, 0);
    
    // Reset local state
    enc_last_counter = 0;
    btn_last_state = true;
}

void Interface_Update(void)
{
    // 1. Check for movement
    int8_t step = Encoder_Get_Step();
    
    // 2. Check for button
    bool is_clicked = Button_Is_Pressed();

    // 3. Update Global Data (Critical Section)
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        // Update button status
        g_system_interface_config.encoder_btn_status = !btn_last_state; // true if held down
        
        // If clicked, update the "single shot" flag (cs_state)
        if (is_clicked) {
             g_system_interface_config.btn_cs_state = true;
        } else {
             g_system_interface_config.btn_cs_state = false;
        }

        // Update encoder value
        // Option A: Just send raw value (0-65535) for monitoring
        g_system_interface_config.encoder_rotate_value = (uint16_t)enc_last_counter;
        
        // Option B: If you want to increment/decrement a virtual value:
        /*
        if (step != 0) {
             // Example: changing a setting
             // g_system_config.reference_value += step * 0.5f;
        }
        */

        osMutexRelease(DataMHandle);
    }
}