 #include "Task/Interface_Task.h"


/* Private Includes */

#include "encoder_config.h"

#include "I2C_LCD.h"

#include "data.hpp"

#include "bmp280.h"

#include "tim.h"

#include "i2c.h"

#include <stdio.h>


/* Zmienne zewnętrzne */

extern osMutexId_t DataMHandle;

extern osSemaphoreId_t BinarySem01Handle;


/* --- Zmienne Lokalne --- */

static uint32_t enc_last_counter = 0;

static float target_temperature = 22.0f;


/* Instancja BMP280 */

BMP280_t bmp280;


/* --- Funkcje Pomocnicze --- */

void IntToString(int num, char* buffer)

{

int i = 0, j = 0;

char temp[10];

if (num == 0) { buffer[0] = '0'; buffer[1] = '\0'; return; }

while (num > 0) { temp[j++] = (num % 10) + '0'; num /= 10; }

while (j > 0) { buffer[i++] = temp[--j]; }

buffer[i] = '\0';

}


static int8_t Encoder_Get_Step(void)

{

uint32_t curr_counter = ENC_ReadCounter(&henc1);

int16_t diff = (int16_t)((uint16_t)curr_counter - (uint16_t)enc_last_counter);

if (diff >= 2 || diff <= -2) {

enc_last_counter = curr_counter;

return (int8_t)(diff / 2);

}

return 0;

}


/* --- Funkcje Publiczne --- */


void Interface_Init(void)

{

// 1. Enkoder

ENC_Init(&henc1);

__HAL_TIM_SET_COUNTER(henc1.Timer, 0);

enc_last_counter = 0;

// 2. LCD

I2C_LCD_Init(I2C_LCD_1);

I2C_LCD_Clear(I2C_LCD_1);

// 3. BMP280 (Inicjalizacja)

// Sprawdź adres 0x76 lub 0x77

bmp280.bmp_i2c = &hi2c2;

bmp280.Address = 0x76;

if (BMP280_Init(&bmp280, &hi2c2, 0x76) != 0)

{

I2C_LCD_WriteString(I2C_LCD_1, "ERR: BMP280");

osDelay(1000);

I2C_LCD_Clear(I2C_LCD_1);

}

// 4. START TIMERA Z PRZERWANIEM

// To uruchamia machinę: Timer -> Przerwanie -> Semafor

HAL_TIM_Base_Start_IT(&htim6);


// Stałe napisy

I2C_LCD_SetCursor(I2C_LCD_1, 0, 0);

I2C_LCD_WriteString(I2C_LCD_1, "Zadana: ");

I2C_LCD_SetCursor(I2C_LCD_1, 0, 1);

I2C_LCD_WriteString(I2C_LCD_1, "Temp: ");

}


void Interface_Update(void)

{

char str_int[10], str_frac[5];



int8_t step = Encoder_Get_Step();

if (step != 0)

{

target_temperature += (float)step * 0.1f;

if (target_temperature < 0.0f) target_temperature = 0.0f;

if (target_temperature > 99.0f) target_temperature = 99.0f;

}


if (osSemaphoreAcquire(BinarySem01Handle, 0) == osOK)

{

// Wchodzimy tu tylko raz na sekundę!

float temp_read = BMP280_ReadTemperature(&bmp280);


// Zapisz do danych globalnych

if (osMutexAcquire(DataMHandle, 10) == osOK)

{

g_system_data.measured_value = temp_read;

osMutexRelease(DataMHandle);

}

}


if (osMutexAcquire(DataMHandle, 10) == osOK)

{

g_system_config.reference_value = target_temperature;

osMutexRelease(DataMHandle);

}


// 1. Zadana

int t_set_val = (int)target_temperature;

int t_set_dec = (int)(((target_temperature - t_set_val) * 10) + 0.5f);

if (t_set_dec >= 10) { t_set_dec = 0; t_set_val++; }

IntToString(t_set_val, str_int);

IntToString(t_set_dec, str_frac);


I2C_LCD_SetCursor(I2C_LCD_1, 8, 0);

I2C_LCD_WriteString(I2C_LCD_1, str_int);

I2C_LCD_WriteChar(I2C_LCD_1, '.');

I2C_LCD_WriteString(I2C_LCD_1, str_frac);

I2C_LCD_WriteString(I2C_LCD_1, " C ");


// 2. Odczytana (z globalnej struktury)

float current_meas = g_system_data.measured_value;

int t_cur_val = (int)current_meas;

int t_cur_dec = (int)(((current_meas - t_cur_val) * 10) + 0.5f);

if (t_cur_dec >= 10) { t_cur_dec = 0; t_cur_val++; }


IntToString(t_cur_val, str_int);

IntToString(t_cur_dec, str_frac);


I2C_LCD_SetCursor(I2C_LCD_1, 8, 1);

I2C_LCD_WriteString(I2C_LCD_1, str_int);

I2C_LCD_WriteChar(I2C_LCD_1, '.');

I2C_LCD_WriteString(I2C_LCD_1, str_frac);

I2C_LCD_WriteString(I2C_LCD_1, " C ");

}

