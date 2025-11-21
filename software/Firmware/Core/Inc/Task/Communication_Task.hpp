
#ifndef _COMMUNICATION_TASK_H_
#define _COMMUNICATION_TASK_H_


//FreeRtos Includes
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "task.h"
#include "semphr.h"
//Other Includes
#include "data.hpp"
#include "usart.h"
#include "main.h"
//#include "Components/Embedded_Json/embedded_json.hpp"



//Function definitions
void StartCommunicationTask(void *argument);

void UART_Com(void);


#endif