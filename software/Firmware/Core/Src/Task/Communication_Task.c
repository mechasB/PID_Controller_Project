/**
  ******************************************************************************
  * @file    Communication_Task.c
  * @brief   Handles UART communication (Telemetry TX and Command RX).
  * Implements interrupt-based reception for PID commands and 
  * polling-based transmission for JSON telemetry.
  ******************************************************************************
  */

#include "Task/Communication_Task.h"

/* --- External Variables --- */
extern osMutexId_t DataMHandle;
extern UART_HandleTypeDef huart2;

/* --- Interrupt Variables --- */
/** Buffer for a single byte received via Interrupt */
static volatile uint8_t rx_byte;          
/** Accumulation buffer for the incoming command string */
static volatile uint8_t rx_buffer[64];    
/** Current index in the accumulation buffer */
static volatile uint8_t rx_index = 0;     
/** Flag: 1 indicates a full command (newline detected) is ready for parsing */
static volatile uint8_t cmd_received = 0; 

/**
 * @brief  Calculates the CRC-8 checksum for data integrity.
 * @param  data   Pointer to the data array.
 * @param  length Length of the data.
 * @return Calculated CRC-8 value.
 */
uint8_t Calculate_CRC8(const char *data, uint16_t length)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc;
}

/**
 * @brief  UART RX Complete Callback (Interrupt Service Routine).
 * Executed automatically when a byte is received.
 * @param  huart Pointer to the UART handle.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // Only process if the previous command has been handled by the main loop
        if (cmd_received == 0)
        {
            // Check for End-Of-Line characters
            if (rx_byte == '\n' || rx_byte == '\r')
            {
                if (rx_index > 0) // Ensure buffer is not empty
                {
                    rx_buffer[rx_index] = 0; // Null-terminate string
                    cmd_received = 1;        // Signal the task to parse
                }
                rx_index = 0; 
            }
            else
            {
                // Store byte in buffer with overflow protection
                if (rx_index < 63) 
                {
                    rx_buffer[rx_index++] = rx_byte;
                }
                else
                {
                    rx_index = 0; // Buffer overflow -> safety reset 
                }
            }
        }
        
        // Re-arm the interrupt to receive the next byte
        HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_byte, 1);
    }
}

/**
 * @brief  Initializes Communication hardware/software.
 * Clears error flags and starts the first interrupt reception.
 */
void Communication_Init(void)
{
    // Clear Overrun Error flag to prevent UART lockup
    __HAL_UART_CLEAR_OREFLAG(&huart2);
    
    // Start listening for the first byte in Interrupt mode
    HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_byte, 1);
}

/**
 * @brief  Main update loop for Communication.
 * 1. Parses incoming PID commands (if flag is set).
 * 2. Sends JSON telemetry every 500ms.
 */
void Communication_Update(void)
{
    static uint32_t last_tx_tick = 0;
    static char tx_buffer[256]; 

    // --- PART 1: COMMAND RECEPTION (Interrupt Logic) ---
    
    // Check if the ISR has flagged a complete command
    if (cmd_received == 1)
    {
        // Expected format: "PID:Kp:Ki:Kd"
        
        if (strncmp((char*)rx_buffer, "PID", 3) == 0)
        {
            // Parse string using tokenization
            char *token = strtok((char*)rx_buffer, ":"); // "PID"
            char *str_kp = strtok(NULL, ":");
            char *str_ki = strtok(NULL, ":");
            char *str_kd = strtok(NULL, ":");

            if (str_kp && str_ki && str_kd)
            {
                float new_kp = (float)atof(str_kp);
                float new_ki = (float)atof(str_ki);
                float new_kd = (float)atof(str_kd);

                // Update global configuration securely
                if (osMutexAcquire(DataMHandle, 10) == osOK) 
                {
                    g_system_config.kp = new_kp;
                    g_system_config.ki = new_ki;
                    g_system_config.kd = new_kd;
                    osMutexRelease(DataMHandle);
                }
            }
        }
        
        // Clear flag to allow ISR to write to buffer again
        cmd_received = 0;
    }

    // --- PART 2: TELEMETRY TRANSMISSION (Every 500ms) ---
    
    uint32_t current_tick = HAL_GetTick();
    if ((current_tick - last_tx_tick) >= 500)
    {
        last_tx_tick = current_tick;
        
        float sp=0, pv=0, cv=0, err=0, k_p=0, k_i=0, k_d=0, pwm_pct=0;
        int fan_state = 0, rdy_state = 0;
        
        // Retrieve latest data securely
        if (osMutexAcquire(DataMHandle, 10) == osOK) {
            sp = g_system_config.reference_value;
            k_p = g_system_config.kp; k_i = g_system_config.ki; k_d = g_system_config.kd;
            pv = g_system_data.measured_value;
            cv = g_system_data.control_signal;
            err = g_system_data.pid_error;
            fan_state = g_system_data.is_fan_on ? 1 : 0;
            rdy_state = g_system_data.is_ready_on ? 1 : 0;
            osMutexRelease(DataMHandle);
        }
        
        // Calculate PWM percentage for display
        pwm_pct = cv / 10.0f;
        if(pwm_pct > 100) pwm_pct=100; if(pwm_pct < 0) pwm_pct=0;

        // Format JSON string
        int len = snprintf(tx_buffer, sizeof(tx_buffer)-10, 
            "{\"ts\":%lu,\"sp\":%.2f,\"pv\":%.2f,\"cv\":%.2f,\"pwm\":%.1f,\"err\":%.2f,\"kp\":%.2f,\"ki\":%.3f,\"kd\":%.2f,\"fan\":%d,\"rdy\":%d}", 
            (unsigned long)current_tick, sp, pv, cv, pwm_pct, err, k_p, k_i, k_d, fan_state, rdy_state);

        // Append CRC and transmit
        if (len > 0) {
            uint8_t crc = Calculate_CRC8(tx_buffer, len);
            int ft = snprintf(tx_buffer+len, sizeof(tx_buffer)-len, "|%02X\r\n", crc);
            if(ft>0) HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len+ft, 100);
        }
    }
}