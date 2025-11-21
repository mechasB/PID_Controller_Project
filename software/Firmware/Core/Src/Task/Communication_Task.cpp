#include "Task/Communication_Task.hpp"


/**
 * @brief Serializuje aktualne dane systemowe do JSON i wysyła przez UART.
 * * Funkcja bezpiecznie odczytuje globalne zmienne chronione Mutexem, 
 * serializuje je do formatu JSON, dodaje znak nowej linii, a następnie 
 * wysyła przez zdefiniowany interfejs UART.
 */
void UART_Com(void)
{
    
}

/* USER CODE BEGIN Header_StartPIDTask */
/**
  * @brief  Function implementing the PID_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartPIDTask */
void StartCommunicationTask(void *argument)
{
  /* USER CODE BEGIN StartPIDTask */

  // Czas oczekiwania pomiędzy transmisjami (np. co 50 ms)
    const TickType_t xDelay = pdMS_TO_TICKS(50);

  /* Infinite loop */
  for(;;)
  {

        UART_Com();
        // Oczekiwanie wymagane w pętli zadania FreeRTOS
        osDelay(xDelay);
  }
  /* USER CODE END StartPIDTask */
}