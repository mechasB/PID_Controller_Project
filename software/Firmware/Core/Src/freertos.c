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
//Include Task 
#include "Task/Interface_Task.h"
#include "Task/Communication_Task.h"
#include "Task/PID_Task.h"
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
  Communication_Init();
  /* Infinite loop */
  for(;;)
  {
    Communication_Update();
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
  Interface_Init();
  /* Infinite loop */
  for(;;)
  {
    Interface_Update();
    osDelay(10);
  }
  /* USER CODE END StartInterfaceTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

