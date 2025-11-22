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
{
    // 1. Inicjalizacja sprzętu
  HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);

  // 2. Zmienne lokalne (pamięć pozycji)
  // 'static' sprawia, że zmienna nie kasuje się przy każdym obiegu pętli
  static int16_t last_encoder_pos = 0; 
  
  // Próg czułości: 4 impulsy to zazwyczaj jeden fizyczny "ząbek" (klik) na enkoderze.
  // Zwiększ to, jeśli dioda reaguje zbyt nerwowo.
  const int16_t ROTATION_THRESHOLD = 4; 

  /* Infinite loop */
  for(;;)
  {

        //int16_t current_pos = (int16_t)__HAL_TIM_GET_COUNTER(&htim8);
        uint32_t current_pos = htim8.Instance->CNT;
        // Zapis do struktury globalnej (dla innych tasków/wyświetlacza)
        g_system_interface_config.encoder_rotate_value = current_pos;

        // B. Logika diody LED (Prawo -> ON, Lewo -> OFF)
        int16_t delta = current_pos - last_encoder_pos;

        if (delta >= ROTATION_THRESHOLD) // Obrót w PRAWO
        {
            // Zapal diodę
            HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, GPIO_PIN_SET);
            
            // Zaktualizuj ostatnią znaną pozycję (punkt odniesienia)
            last_encoder_pos = current_pos; 
        }
        else if (delta <= -ROTATION_THRESHOLD) // Obrót w LEWO
        {
            // Zgaś diodę
            HAL_GPIO_WritePin(READY_LED_GPIO_Port, READY_LED_Pin, GPIO_PIN_RESET);
            
            // Zaktualizuj ostatnią znaną pozycję
            last_encoder_pos = current_pos;
        }

        // C. Obsługa przycisku (bez zmian, tylko dodana do Mutexa)
        if (HAL_GPIO_ReadPin(ENCODER_BTN_GPIO_Port, ENCODER_BTN_Pin) == GPIO_PIN_RESET) {
            g_system_interface_config.btn_cs_state = true;
        } else {
            g_system_interface_config.btn_cs_state = false;
        }

        if (g_system_interface_config.btn_cs_state == 1)
        {
          HAL_GPIO_TogglePin(HEAT_LED_GPIO_Port, HEAT_LED_Pin);
        }
        

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
  char tx_buffer[64];
  int16_t encoder_val_to_send = 0;
  /* Infinite loop */
  for(;;)
  {
// 1. ODCZYT DANYCH (SEKCJA KRYTYCZNA)
    // Chcemy odczytać g_system_interface_config, więc musimy wziąć ten sam klucz co PID_Task
    if (osMutexAcquire(DataMHandle, 10) == osOK)
    {
        encoder_val_to_send = g_system_interface_config.encoder_rotate_value;
        
        // Oddajemy klucz natychmiast po skopiowaniu wartości!
        // Nie trzymaj klucza podczas wysyłania UART, bo zablokujesz PID_Task na długo.
        osMutexRelease(DataMHandle);
    }

    // 2. PRZYGOTOWANIE TEKSTU
    // %d oznacza liczbę całkowitą (int)
    // \r\n to znak nowej linii (żeby w terminalu dane były pod sobą)
    int len = sprintf(tx_buffer, "Encoder: %d\r\n", encoder_val_to_send);

    // 3. WYSYŁKA UART
    // Używamy huart2 (standard dla USB w Nucleo).
    // Jeśli Twoja płytka używa innego UARTu, zmień &huart2 na &huart1 lub &huart3.
    if (len > 0) 
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, 100);
    }

    // 4. OPÓŹNIENIE
    // 100ms = 10 razy na sekundę. To wystarczająco dla oka, a nie zapycha łącza.
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

