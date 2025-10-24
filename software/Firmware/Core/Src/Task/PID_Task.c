#include "Task/PID_Task.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "task.h"


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
    osDelay(1);
  }
  /* USER CODE END StartPIDTask */
}