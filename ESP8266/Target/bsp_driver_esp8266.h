/*
 * bsp_driver_esp8266.h
 *
 *  Created on: Oct 19, 2019
 *      Author: harganda
 */

#ifndef __BSP_DRIVER_ESP8266_H_
#define __BSP_DRIVER_ESP8266_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdarg.h>

/* Board specific includes */
#include "stm32746g_discovery.h"
#include "stm32f7xx_hal_uart.h"

#define BSP_ESP8266_BUFFER_LEN (128)
#define BSP_ESP8266_QUEUE_LEN  (16)

/* Enable task usage, will dynamically allocate strings */
#define configBSP_ESP8266_ENABLE_TASK (1)

 /**
  * @brief  Initialize UART port to use as ESP8266 communications port.
  * @param  pxUARTHandler: UART Handler pointer to use as CLI.
 * @retval : Initialization status (OK: 1, Error: 0).
  */
uint8_t BSP_ESP8266_Init(UART_HandleTypeDef* pxUARTHandler);

/**
 * @brief  Reset ESP8266 module.
 * @param  None.
 * @retval : None.
 */
void BSP_ESP8266_Reset(void);

/**
  * @brief  Write bytes to ESP8266
  * @param  src: Pointer to data source.
  * @param  len: Source length.
 * @retval : Number of bytes written.
 */
uint16_t BSP_ESP8266_Write(const char* src, uint16_t len);

/**
  * @brief  Read bytes from ESP8266
  * @param  dst: Pointer to data destination.
  * @param  len: Destination available space.
 * @retval : Number of bytes read.
 */
uint16_t BSP_ESP8266_Read(const char* dst, uint16_t len);

/**
 * @brief  Print to ESP8266 with format.
 * @param  fmt: Formatter string.
 * @retval : Number of bytes written.
 */
uint16_t BSP_ESP8266_Printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_DRIVER_ESP8266_H_ */
