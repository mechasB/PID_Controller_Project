/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "data.hpp"
#include "usart.h"  
#include "tim.h"
#include <stdio.h>
#include "encoder_config.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
SystemData_t g_system_data;
SystemConfig_t g_system_config;
SystemInterfaceConfig_t g_system_interface_config;
/* USER CODE END Variables */
/* Definitions for PID_Task */
osThreadId_t PID_TaskHandle;
const osThreadAttr_t PID_Task_attributes = {
  .name = "PID_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Communication_T */
osThreadId_t Communication_THandle;
const osThreadAttr_t Communication_T_attributes = {
  .name = "Communication_T",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Interface_Task */
osThreadId_t Interface_TaskHandle;
const osThreadAttr_t Interface_Task_attributes = {
  .name = "Interface_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for DataM */
osMutexId_t DataMHandle;
const osMutexAttr_t DataM_attributes = {
  .name = "DataM"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartPIDTask(void *argument);
void StartCommunicationTask(void *argument);
void StartInterfaceTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of DataM */
  DataMHandle = osMutexNew(&DataM_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  // g_data_mutex = xSemaphoreCreateMutex();

  // if (g_data_mutex == NULL) {
  //     Error_Handler(); 
  // }
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  //g_DataSemaphore = xSemaphoreCreateBinary();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of PID_Task */
  PID_TaskHandle = osThreadNew(StartPIDTask, NULL, &PID_Task_attributes);

  /* creation of Communication_T */
  Communication_THandle = osThreadNew(StartCommunicationTask, NULL, &Communication_T_attributes);

  /* creation of Interface_Task */
  Interface_TaskHandle = osThreadNew(StartInterfaceTask, NULL, &Interface_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartPIDTask */
/**
  * @brief  Function implementing the PID_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartPIDTask */
void StartPIDTask(void *argument)
{
  /* USER CODE BEGIN StartPIDTask */

  /* Infinite loop */
  for(;;)
  {
    
    osDelay(10);
  }
  /* USER CODE END StartPIDTask */
}

/* USER CODE BEGIN Header_StartCommunicationTask */
/**
* @brief Function implementing the Communication_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommunicationTask */
void StartCommunicationTask(void *argument)
{
  /* USER CODE BEGIN StartCommunicationTask */
  char tx_buffer[64]; 
  uint32_t val_to_send = 0;
  /* Infinite loop */
  for(;;)
  {
// 1. Pobranie danych (krótki odczyt pod Mutexem)
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        val_to_send = g_system_interface_config.encoder_rotate_value;
        osMutexRelease(DataMHandle);
    }

    // 2. Formatowanie JSON
    // Używamy snprintf (bezpieczniejszy). 
    // Format: {"encoder":123} + \r\n (powrót karetki i nowa linia)
    int len = snprintf(tx_buffer, sizeof(tx_buffer), "{\"encoder\":%lu}\r\n", val_to_send);

    // 3. Wysyłka UART
    if (len > 0)
    {
        // Upewnij się, że to ten sam UART co masz podpięty do USB (huart2 lub huart3)
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, 100);
    }

    // 4. Opóźnienie (10Hz - płynny wykres, nie zapycha łącza)
    osDelay(100);
  }
  /* USER CODE END StartCommunicationTask */
}

/* USER CODE BEGIN Header_StartInterfaceTask */
/**
* @brief Function implementing the Interface_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInterfaceTask */
void StartInterfaceTask(void *argument)
{
  /* USER CODE BEGIN StartInterfaceTask */
  ENC_Init(&henc1);
  /* Infinite loop */
  for(;;)
  {
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        // A. Odczyt Enkodera
        // Zapisujemy surową wartość uint32_t (zgodnie z Twoim wzorcowym main.c)
        g_system_interface_config.encoder_rotate_value = (uint16_t)ENC_ReadCounter(&henc1);

        // B. Odczyt Przycisku
        if (HAL_GPIO_ReadPin(ENCODER_BTN_GPIO_Port, ENCODER_BTN_Pin) == GPIO_PIN_RESET) {
            g_system_interface_config.btn_cs_state = true;
        } else {
            g_system_interface_config.btn_cs_state = false;
        }

        // C. Logika lokalna (np. LED reagujący na przycisk)
        if (g_system_interface_config.btn_cs_state == true)
        {
             // Tu możesz np. zerować enkoder przyciskiem
             // __HAL_TIM_SET_COUNTER(henc1.Timer, 0);
        }

        // Zwalniamy zasoby
        osMutexRelease(DataMHandle);
    }
    
    osDelay(20);
  }
  /* USER CODE END StartInterfaceTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

