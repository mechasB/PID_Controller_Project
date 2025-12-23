#ifndef COMMUNICATION_TASK_H
#define COMMUNICATION_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  ******************************************************************************
  * @file    Communication_Task.h
  * @brief   Header file for the Communication Task (UART Telemetry & Commands).
  *
  * This file contains function prototypes for initializing the UART communication
  * in interrupt mode and handling the cyclic data transmission/reception logic.
  ******************************************************************************
  */

/* --- Includes --- */
#include "main.h"
#include "data.hpp"    /* Global system data structures */
#include "usart.h"     /* UART handle access */
#include "cmsis_os2.h" /* RTOS types (osMutexId_t) */
#include <stdio.h>     /* Standard I/O for snprintf */
#include <string.h>
#include <stdlib.h> 

/* --- Public Function Prototypes --- */

/**
 * @brief  Initializes the communication module.
 *
 * This function performs the following actions:
 * - Clears UART error flags (Overrun Error).
 * - Enables UART Receive Interrupt (RX_IT) to start listening for commands.
 * * @note   This function must be called once before starting the FreeRTOS scheduler.
 * @param  None
 * @retval None
 */
void Communication_Init(void);

/**
 * @brief  Main update loop for the Communication Task.
 *
 * This function handles the logic for:
 * 1. Processing received commands (parsing PID values from UART buffer).
 * 2. Sending telemetry data (JSON format) periodically (e.g., every 500ms).
 *
 * @note   This function should be called cyclically inside the FreeRTOS task loop.
 * @param  None
 * @retval None
 */
void Communication_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMUNICATION_TASK_H */