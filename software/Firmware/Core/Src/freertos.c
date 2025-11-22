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

#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "data.hpp"
#include "tim.h"
//#include "i2c_lcd.h"
//#include "i2c.h"
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
int16_t encoder_rotate_value;
bool encoder_btn_status;
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
/* Definitions for DataSemaphore */
osSemaphoreId_t DataSemaphoreHandle;
const osSemaphoreAttr_t DataSemaphore_attributes = {
  .name = "DataSemaphore"
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

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of DataSemaphore */
  DataSemaphoreHandle = osSemaphoreNew(1, 1, &DataSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
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
  HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
  __HAL_TIM_SET_COUNTER(&htim8, 0);
  /* Infinite loop */
  for(;;)
  {
    //ODCZYT ENKODERA
    encoder_rotate_value = (int16_t)__HAL_TIM_GET_COUNTER(&htim8);

    // ODCZYT PRZYCISKU
    // Jeśli stan niski (RESET) to true (wciśnięty)
    if (HAL_GPIO_ReadPin(ENCODER_BTN_GPIO_Port, ENCODER_BTN_Pin) == GPIO_PIN_RESET) {
        encoder_btn_status = true;
    } else {
        encoder_btn_status = false;
    }

    // TEST INTERAKCJI (Reset licznika przy wciśnięciu)
    if (encoder_btn_status == true)
    {
      HAL_GPIO_TogglePin(READY_LED_GPIO_Port, READY_LED_Pin);
    }

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
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
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
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartInterfaceTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

