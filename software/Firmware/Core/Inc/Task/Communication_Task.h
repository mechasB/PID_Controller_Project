#ifndef COMMUNICATION_TASK_H
#define COMMUNICATION_TASK_H

/* Include plików systemowych */
#include "main.h"     // Definicje HAL
#include "data.hpp"   // Twoje struktury (lub data.h jeśli też zmieniłeś)
#include "usart.h"    // Dostęp do huart3
#include "cmsis_os2.h" // Dostęp do typów RTOS (osMutexId_t)
#include <stdio.h>    // Do snprintf

/* Funkcja inicjalizująca (jeśli potrzebna w przyszłości) */
void Communication_Init(void);

/* Główna logika wysyłania - wywoływana w pętli taska */
void Communication_Update(void);

#endif /* COMMUNICATION_TASK_H */