/**
  ******************************************************************************
  * @file    Interface_Task.hpp
  * @brief   Header for Interface Task - Encoder & Button handling only.
  ******************************************************************************
  */

#ifndef INTERFACE_TASK_HPP_
#define INTERFACE_TASK_HPP_

/* Private Includes */
#include "encoder_config.h" 
#include "I2C_LCD.h"        
#include "data.hpp"         
#include "bmp280.h"         
#include "tim.h"            
#include "i2c.h"            
#include <stdio.h>       

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initializes the encoder hardware.
 * Call this once before the task loop.
 */
void Interface_Init(void);

/**
 * @brief  Reads Encoder and Button state.
 * Updates global data structure. Call this cyclically.
 */
void Interface_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_TASK_HPP_ */