#ifndef COMMUNICATION_TASK_H
#define COMMUNICATION_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* --- Includes --- */
#include "main.h"
#include "data.hpp"   // Twoje struktury (lub data.h jeśli też zmieniłeś)
#include "usart.h"    // Dostęp do huart3
#include "cmsis_os2.h" // Dostęp do typów RTOS (osMutexId_t)
#include <stdio.h>    // Do snprintf

/* --- Public Function Prototypes --- */

/**
 * @brief Inicjalizacja komunikacji UART (włączenie przerwań Rx).
 * Wywołać raz przed startem schedulera FreeRTOS.
 */
void Communication_Init(void);

/**
 * @brief Główna funkcja zadania komunikacyjnego.
 * Wywoływać cyklicznie wewnątrz tasku FreeRTOS.
 */
void Communication_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMUNICATION_TASK_H */