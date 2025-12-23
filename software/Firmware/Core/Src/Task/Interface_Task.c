/**
  ******************************************************************************
  * @file    Interface_Task.c
  * @brief   Manages the User Interface: LCD Display, Encoder Input, 
  * and BMP280 Sensor Reading.
  ******************************************************************************
  */

#include "Task/Interface_Task.h"

/* External Variables */
extern osMutexId_t DataMHandle;
extern osSemaphoreId_t BinarySem01Handle;

/* --- Local Variables --- */
static uint32_t enc_last_counter = 0;
static float target_temperature = 22.0f;

/* BMP280 Instance */
BMP280_t bmp280;

/* --- Helper Functions --- */

/**
 * @brief  Converts integer to string manually (lightweight implementation).
 * @param  num    Integer to convert.
 * @param  buffer Output buffer.
 */
void IntToString(int num, char* buffer)
{
    int i = 0, j = 0;
    char temp[10];

    if (num == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while (num > 0)
    {
        temp[j++] = (num % 10) + '0';
        num /= 10;
    }

    while (j > 0)
    {
        buffer[i++] = temp[--j];
    }
    buffer[i] = '\0';
}

/**
 * @brief  Calculates the steps moved by the encoder.
 * @return Delta steps (divide by 2 for stability).
 */
static int8_t Encoder_Get_Step(void)
{
    uint32_t curr_counter = ENC_ReadCounter(&henc1);
    int16_t diff = (int16_t)((uint16_t)curr_counter - (uint16_t)enc_last_counter);

    if (diff >= 2 || diff <= -2)
    {
        enc_last_counter = curr_counter;
        return (int8_t)(diff / 2);
    }
    return 0;
}

/* --- Public Functions --- */

/**
 * @brief  Initializes LCD, Encoder, and Sensor.
 */
void Interface_Init(void)
{
    // 1. Encoder Init
    ENC_Init(&henc1);
    __HAL_TIM_SET_COUNTER(henc1.Timer, 0);
    enc_last_counter = 0;

    // 2. LCD Init
    I2C_LCD_Init(I2C_LCD_1);
    I2C_LCD_Clear(I2C_LCD_1);

    // 3. BMP280 Init
    bmp280.bmp_i2c = &hi2c2;
    bmp280.Address = 0x76;

    if (BMP280_Init(&bmp280, &hi2c2, 0x76) != 0)
    {
        I2C_LCD_WriteString(I2C_LCD_1, "ERR: BMP280");
        osDelay(1000);
        I2C_LCD_Clear(I2C_LCD_1);
    }

    // 4. Start Timer Interrupt (Triggers Sensor Semaphore)
    HAL_TIM_Base_Start_IT(&htim6);

    // Static LCD Text
    I2C_LCD_SetCursor(I2C_LCD_1, 0, 0);
    I2C_LCD_WriteString(I2C_LCD_1, "Zadana: ");
    I2C_LCD_SetCursor(I2C_LCD_1, 0, 1);
    I2C_LCD_WriteString(I2C_LCD_1, "Temp: ");
}

/**
 * @brief  Updates the Interface logic.
 * Called cyclically by the FreeRTOS task.
 */
void Interface_Update(void)
{
    char str_int[10], str_frac[5];

    // --- Encoder Handling ---
    int8_t step = Encoder_Get_Step();
    if (step != 0)
    {
        target_temperature += (float)step * 0.1f;
        // Clamp values
        if (target_temperature < 0.0f) target_temperature = 0.0f;
        if (target_temperature > 99.0f) target_temperature = 99.0f;
    }

    // --- Sensor Reading (Synced via Semaphore, e.g., 1Hz) ---
    if (osSemaphoreAcquire(BinarySem01Handle, 0) == osOK)
    {
        // Executes only when Timer Interrupt releases the semaphore
        float temp_read = BMP280_ReadTemperature(&bmp280);

        // Update global measured value
        if (osMutexAcquire(DataMHandle, 10) == osOK)
        {
            g_system_data.measured_value = temp_read;
            osMutexRelease(DataMHandle);
        }
    }

    // --- Update Global Setpoint ---
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        g_system_config.reference_value = target_temperature;
        osMutexRelease(DataMHandle);
    }

    // --- LCD Rendering ---

    // 1. Setpoint (Line 0)
    int t_set_val = (int)target_temperature;
    int t_set_dec = (int)(((target_temperature - t_set_val) * 10) + 0.5f);
    if (t_set_dec >= 10)
    {
        t_set_dec = 0;
        t_set_val++;
    }

    IntToString(t_set_val, str_int);
    IntToString(t_set_dec, str_frac);

    I2C_LCD_SetCursor(I2C_LCD_1, 8, 0);
    I2C_LCD_WriteString(I2C_LCD_1, str_int);
    I2C_LCD_WriteChar(I2C_LCD_1, '.');
    I2C_LCD_WriteString(I2C_LCD_1, str_frac);
    I2C_LCD_WriteString(I2C_LCD_1, " C ");

    // 2. Measured (Line 1)
    float current_meas = g_system_data.measured_value;
    int t_cur_val = (int)current_meas;
    int t_cur_dec = (int)(((current_meas - t_cur_val) * 10) + 0.5f);
    if (t_cur_dec >= 10)
    {
        t_cur_dec = 0;
        t_cur_val++;
    }

    IntToString(t_cur_val, str_int);
    IntToString(t_cur_dec, str_frac);

    I2C_LCD_SetCursor(I2C_LCD_1, 8, 1);
    I2C_LCD_WriteString(I2C_LCD_1, str_int);
    I2C_LCD_WriteChar(I2C_LCD_1, '.');
    I2C_LCD_WriteString(I2C_LCD_1, str_frac);
    I2C_LCD_WriteString(I2C_LCD_1, " C ");
}