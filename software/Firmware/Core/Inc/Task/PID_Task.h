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
#include "data.hpp"       // Global data structures
#include "tim.h"          // PWM Timer (htim3)
#include "gpio.h"         // Fan GPIO
#include "main.h"         // HAL_GetTick, HAL_Delay
#include <math.h>  

#define PWM_PERIOD_ARR      1000.0f  // Wartość AutoReload Timera (ARR)
#define PID_OUT_MAX         PWM_PERIOD_ARR
#define PID_OUT_MIN         0.0f

#ifndef TASK_PID_TASK_H_
#define TASK_PID_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

// Function Prototypes
void PID_Init(void);
void PID_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_PID_TASK_H_ */

#endif