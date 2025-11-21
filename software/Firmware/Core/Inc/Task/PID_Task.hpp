#ifndef _PID_TASK_H
#define _PID_TASK_H

//FreeRtos Includes
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "task.h"
#include "semphr.h"
//Other Includes
#include "data.hpp"
#include "main.h"

/* USER CODE BEGIN Header_StartPIDTask */
/**
  * @brief  Function implementing the PID_Task thread.
  * @param  argument: Not used
  * @retval None
  */
void StartPIDTask(void *argument);


#endif