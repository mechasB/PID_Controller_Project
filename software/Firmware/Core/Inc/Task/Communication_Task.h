//FreeRtos Includes
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "task.h"
//Other Includes
#include "data.h"
#include "usart.h"
#include "main.h"



//Function definitions
void StartCommunicationTask(void *argument);

void UART_Com(void);