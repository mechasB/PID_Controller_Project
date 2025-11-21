#ifndef _INTERFACE_TASK_
#define _INTERFACE_TASK_

//FreeRtos Includes
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "task.h"
#include "semphr.h"
//Other Includes
#include "adc.h"
#include "usart.h"
#include "gpio.h"
#include "main.h"
#include "data.hpp"


/* USER CODE BEGIN Header_StartInterfaceTask */
/**
* @brief Function implementing the Interface_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInterfaceTask */
void StartInterfaceTask(void *argument);

#endif 