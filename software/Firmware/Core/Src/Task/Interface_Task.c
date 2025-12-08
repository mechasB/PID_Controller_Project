#include "Task/Interface_Task.h"

/* Private Includes */
#include "encoder_config.h" 
#include "I2C_LCD.h"        
#include "data.hpp"         

/* Zmienne zewnętrzne */
extern osMutexId_t DataMHandle; 

/* --- Zmienne Lokalne --- */
static uint32_t enc_last_counter = 0;
static float target_temperature = 25.0f; 

/* --- Funkcje Pomocnicze --- */

void IntToString(int num, char* buffer) 
{
    int i = 0;
    int j = 0;
    char temp[10];

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while (num > 0) {
        temp[j++] = (num % 10) + '0';
        num /= 10;
    }

    while (j > 0) {
        buffer[i++] = temp[--j];
    }
    buffer[i] = '\0';
}

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

/* --- Funkcje Publiczne --- */

void Interface_Init(void)
{
    ENC_Init(&henc1);
    __HAL_TIM_SET_COUNTER(henc1.Timer, 0);
    enc_last_counter = 0;
    
    I2C_LCD_Init(I2C_LCD_1);
    I2C_LCD_Clear(I2C_LCD_1);
    
    // Rysujemy stałe elementy
    I2C_LCD_SetCursor(I2C_LCD_1, 0, 0);
    I2C_LCD_WriteString(I2C_LCD_1, "Zadana: ");
    
    I2C_LCD_SetCursor(I2C_LCD_1, 0, 1);
    I2C_LCD_WriteString(I2C_LCD_1, "Temp:   ");
}

void Interface_Update(void)
{
    char str_int[10];
    char str_frac[5];

    // --- 1. Enkoder ---
    int8_t step = Encoder_Get_Step();
    if (step != 0)
    {
        // ZMIANA: Krok 0.1 stopnia
        target_temperature += (float)step * 0.1f;
        
        // Limity
        if (target_temperature < 0.0f)   target_temperature = 0.0f;
        if (target_temperature > 99.0f) target_temperature = 99.0f;
    }

    // --- 2. Synchronizacja ---
    float current_meas = 0.0f;
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        current_meas = g_system_data.measured_value;
        g_system_config.reference_value = target_temperature;
        osMutexRelease(DataMHandle);
    }

    // --- 3. Wyświetlanie ZADANEJ (Linia 0) ---
    int t_set_val = (int)target_temperature;
    
    // POPRAWKA ZAOKRĄGLANIA: Dodajemy 0.5f przed rzutowaniem na int.
    // Dzięki temu 0.09999 zamieni się na 0.1 -> 1, a nie 0.
    int t_set_dec = (int)(((target_temperature - t_set_val) * 10) + 0.5f);
    
    // Zabezpieczenie przed "wskoczeniem" dziesiątki (gdyby zaokrąglenie dało 10)
    if (t_set_dec >= 10) { t_set_dec = 0; t_set_val++; }
    if (t_set_dec < 0) t_set_dec = 0;

    IntToString(t_set_val, str_int);
    IntToString(t_set_dec, str_frac);

    I2C_LCD_SetCursor(I2C_LCD_1, 8, 0); 
    I2C_LCD_WriteString(I2C_LCD_1, str_int);
    I2C_LCD_WriteChar(I2C_LCD_1, '.');
    I2C_LCD_WriteString(I2C_LCD_1, str_frac);
    I2C_LCD_WriteString(I2C_LCD_1, " C  ");

    // --- 4. Wyświetlanie OBECNEJ (Linia 1) ---
    int t_cur_val = (int)current_meas;
    int t_cur_dec = (int)(((current_meas - t_cur_val) * 10) + 0.5f);
    
    if (t_cur_dec >= 10) { t_cur_dec = 0; t_cur_val++; }
    if (t_cur_dec < 0) t_cur_dec = 0;

    IntToString(t_cur_val, str_int);
    IntToString(t_cur_dec, str_frac);

    I2C_LCD_SetCursor(I2C_LCD_1, 8, 1);
    I2C_LCD_WriteString(I2C_LCD_1, str_int);
    I2C_LCD_WriteChar(I2C_LCD_1, '.');
    I2C_LCD_WriteString(I2C_LCD_1, str_frac);
    I2C_LCD_WriteString(I2C_LCD_1, " C  ");
}