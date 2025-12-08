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

/* --- Funkcje Pomocnicze (Konwersja bez sprintf) --- */

/**
 * @brief Zamienia liczbę całkowitą (dodatnią) na tekst.
 * @param num: Liczba do konwersji (np. 25)
 * @param buffer: Bufor wyjściowy (np. "25")
 */
void IntToString(int num, char* buffer) 
{
    int i = 0;
    int j = 0;
    char temp[10]; // Bufor tymczasowy na odwrócone cyfry

    // Obsługa zera
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    // Wyciąganie cyfr od końca
    while (num > 0) {
        temp[j++] = (num % 10) + '0'; // Zamiana cyfry na znak ASCII
        num /= 10;
    }

    // Odwracanie kolejności (z temp do buffer)
    while (j > 0) {
        buffer[i++] = temp[--j];
    }
    buffer[i] = '\0'; // Znak końca tekstu
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
    
    // Inicjalizacja LCD
    I2C_LCD_Init(I2C_LCD_1);
    I2C_LCD_Clear(I2C_LCD_1);
    
    // Rysujemy stałe napisy RAZ na początku (żeby nie mrugały)
    I2C_LCD_SetCursor(I2C_LCD_1, 0, 0);
    I2C_LCD_WriteString(I2C_LCD_1, "Zadana: ");
    
    I2C_LCD_SetCursor(I2C_LCD_1, 0, 1);
    I2C_LCD_WriteString(I2C_LCD_1, "Temp:   ");
}

void Interface_Update(void)
{
    float measured_temperature = 0.0f;
    
    // Bufory na tekst liczb (np. "25" i "5")
    char str_int[10];
    char str_frac[5];

    // --- 1. Enkoder ---
    int8_t step = Encoder_Get_Step();
    if (step != 0)
    {
        target_temperature += (float)step * 0.5f;
        if (target_temperature < 0.0f)   target_temperature = 0.0f;
        if (target_temperature > 99.0f) target_temperature = 99.0f; // Limit 99 dla czytelności LCD
    }

    // --- 2. Synchronizacja ---
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        measured_temperature = g_system_data.measured_value;
        g_system_config.reference_value = target_temperature;
        osMutexRelease(DataMHandle);
    }

    // --- 3. Wyświetlanie (Bez sprintf) ---

    // A. Wyświetlanie ZADANEJ (Linia 0)
    // Rozbijamy float: 25.5 -> 25 i 5
    int t_set_val = (int)target_temperature;
    int t_set_dec = (int)((target_temperature - t_set_val) * 10);
    if(t_set_dec < 0) t_set_dec = -t_set_dec;

    // Konwersja na tekst
    IntToString(t_set_val, str_int);
    IntToString(t_set_dec, str_frac);

    // Wypisywanie po kawałku (Cursor ustawiamy za napisem "Zadana: ")
    I2C_LCD_SetCursor(I2C_LCD_1, 8, 0); 
    I2C_LCD_WriteString(I2C_LCD_1, str_int);   // np. "25"
    I2C_LCD_WriteChar(I2C_LCD_1, '.');         // "."
    I2C_LCD_WriteString(I2C_LCD_1, str_frac);  // "5"
    I2C_LCD_WriteString(I2C_LCD_1, " C  ");    // Spacje czyszczą stare śmieci

    // B. Wyświetlanie OBECNEJ (Linia 1)
    int t_cur_val = (int)measured_temperature;
    int t_cur_dec = (int)((measured_temperature - t_cur_val) * 10);
    if(t_cur_dec < 0) t_cur_dec = -t_cur_dec;

    IntToString(t_cur_val, str_int);
    IntToString(t_cur_dec, str_frac);

    // Wypisywanie po kawałku (Cursor ustawiamy za napisem "Temp:   ")
    I2C_LCD_SetCursor(I2C_LCD_1, 8, 1);
    I2C_LCD_WriteString(I2C_LCD_1, str_int);
    I2C_LCD_WriteChar(I2C_LCD_1, '.');
    I2C_LCD_WriteString(I2C_LCD_1, str_frac);
    I2C_LCD_WriteString(I2C_LCD_1, " C  ");
}