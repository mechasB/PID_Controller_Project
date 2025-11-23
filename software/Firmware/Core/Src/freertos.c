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
// 1. Inicjalizacja biblioteki enkodera (zastępuje HAL_TIM_Encoder_Start)
  // Funkcja ta uruchamia timer zdefiniowany w henc1 (czyli htim8)
  ENC_Init(&henc1); 
  
  // Opcjonalnie: Ustawienie licznika na 0 na start (biblioteka ma do tego funkcję)
  // Uwaga: W pliku encoder.c funkcja nazywa się ENC_SetCounter, a w .h ENC_WriteCounter.
  // Użyjemy wersji bezpośredniej na timerze dla pewności, dopóki nie poprawisz literówki w bibliotece.
  __HAL_TIM_SET_COUNTER(henc1.Timer, 0);

  static int16_t last_encoder_pos = 0; 
  const int16_t ROTATION_THRESHOLD = 2; // Czułość

  /* Infinite loop */
  for(;;)
  {

// --- SEKCJA KRYTYCZNA ---
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        // 1. ODCZYT Z BIBLIOTEKI
        // ENC_ReadCounter zwraca uint32_t. Rzutujemy na int16_t, aby zachować
        // logikę wykrywania kierunku przy przejściu przez zero (0 -> 65535).
        int16_t current_pos = (int16_t)ENC_ReadCounter(&henc1);

        // Zapis do struktury globalnej
        g_system_interface_config.encoder_rotate_value = current_pos;

        // 2. LOGIKA DIODY (Delta)
        int16_t delta = current_pos - last_encoder_pos;

        if (delta >= ROTATION_THRESHOLD) // Obrót w PRAWO
        {
            HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, GPIO_PIN_SET);
            last_encoder_pos = current_pos;
        }
        else if (delta <= -ROTATION_THRESHOLD) // Obrót w LEWO
        {
            HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, GPIO_PIN_RESET);
            last_encoder_pos = current_pos;
        }

        // 3. OBSŁUGA PRZYCISKU
        if (HAL_GPIO_ReadPin(ENCODER_BTN_GPIO_Port, ENCODER_BTN_Pin) == GPIO_PIN_RESET) {
            g_system_interface_config.btn_cs_state = true;
        } else {
            g_system_interface_config.btn_cs_state = false;
        }

        // Reset licznika przyciskiem
        if (g_system_interface_config.btn_cs_state == true)
        {
             // Reset przez bibliotekę (lub bezpośrednio na timerze)
             __HAL_TIM_SET_COUNTER(henc1.Timer, 0);
             henc1.Counter = 0; // Ważne: zaktualizuj też strukturę biblioteki!
             last_encoder_pos = 0;
        }

        osMutexRelease(DataMHandle);
    }

    // Krótkie opóźnienie (np. 10ms = 100Hz odświeżania)
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
  
  char tx_buffer[64]; // Bufor na tekst
  int16_t val_to_send = 0;

  /* Infinite loop */
  for(;;)
  {
    // 1. Pobranie danych (krótki czas blokady mutexa)
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        val_to_send = g_system_interface_config.encoder_rotate_value;
        osMutexRelease(DataMHandle);
    }

    // 2. Formatowanie i wysyłka
    // Format: "ENC: <wartość>" + nowa linia
    int len = sprintf(tx_buffer, "ENC: %d\r\n", val_to_send);

    if (len > 0)
    {
        // Używamy &huart2 (USB w Nucleo). Sprawdź w usart.c czy to właściwy uchwyt.
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, 100);
    }

    // 3. Opóźnienie (np. 100ms = 10Hz odświeżania w terminalu)
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

