/**
  ******************************************************************************
  * @file    Interface_Task.c
  * @brief   Prosty sterownik 
  ******************************************************************************
  */

#include "Task/Interface_Task.h"

/* Private Includes */
#include "encoder_config.h" 
#include "i2c_lcd.h"        
#include "data.hpp"         
#include <stdio.h>          

/* Zmienne zewnętrzne */
extern osMutexId_t DataMHandle; 

/* --- Zmienne Lokalne --- */
static uint32_t enc_last_counter = 0;
I2C_LCD_HandleTypeDef hlcd; 
// Temperatura zadana (startujemy np. od 25 stopni)
static float target_temperature = 23.0f; 

/* --- Funkcje Pomocnicze --- */

/**
 * @brief Oblicza zmianę pozycji enkodera (delta).
 * Zwraca: +1 (prawo), -1 (lewo), 0 (brak ruchu)
 */
static int8_t Encoder_Get_Step(void)
{
    uint32_t curr_counter = ENC_ReadCounter(&henc1);
    
    // Oblicz różnicę z uwzględnieniem przekręcenia licznika 16-bit
    int16_t diff = (int16_t)((uint16_t)curr_counter - (uint16_t)enc_last_counter);

    // Próg czułości (2 impulsy = 1 krok temperatury)
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
    // 1. Inicjalizacja sprzętu
    ENC_Init(&henc1);
    
    hlcd.hi2c = &hi2c1;  
    hlcd.address = 0x27; // Adres I2C wyświetlacza (zmień na 0x7E jeśli nie działa)
    lcd_init(&hlcd);     
    lcd_clear(&hlcd);
    
    // Reset zmiennych
    __HAL_TIM_SET_COUNTER(henc1.Timer, 0);
    enc_last_counter = 0;
}

void Interface_Update(void)
{
    float measured_temperature = 0.0f;
    char lcd_buf[17]; // Bufor na tekst (16 znaków + koniec linii)

    // --- KROK 1: Synchronizacja Danych (Mutex) ---
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        // 1. Pobierz aktualną temperaturę z czujnika
        measured_temperature = g_system_data.measured_value;

        // 2. Wyślij naszą zadaną temperaturę do reszty systemu
        // (żeby regulator PID wiedział do czego dążyć)
        g_system_config.reference_value = target_temperature;
        
        // (Opcjonalnie) wysyłamy pozycję enkodera do debugu UART
        g_system_interface_config.encoder_rotate_value = (uint16_t)enc_last_counter;

        osMutexRelease(DataMHandle);
    }

    // --- KROK 2: Obsługa Enkodera (Zmiana Zadanej) ---
    int8_t step = Encoder_Get_Step();
    if (step != 0)
    {
        // Zmień temperaturę o 0.5 stopnia na każdy "klik"
        target_temperature += (float)step * 0.5f;

        // Opcjonalne ograniczenia (np. min 0, max 100 stopni)
        if (target_temperature < 0.0f) target_temperature = 0.0f;
        if (target_temperature > 100.0f) target_temperature = 100.0f;
    }

    // --- KROK 3: Wyświetlanie (Stałe Odświeżanie) ---
    
    // Linia 1: Temperatura Zadana (SET)
    // \xDF to symbol stopnia (°) na wyświetlaczach LCD
    sprintf(lcd_buf, "Zadana: %5.1f \xDF""C", target_temperature);
    lcd_gotoxy(&hlcd, 0, 0);
    lcd_puts(&hlcd, lcd_buf);

    // Linia 2: Temperatura Odczytana (ACT)
    sprintf(lcd_buf, "Odczyt: %5.1f \xDF""C", measured_temperature);
    lcd_gotoxy(&hlcd, 0, 1);
    lcd_puts(&hlcd, lcd_buf);
}