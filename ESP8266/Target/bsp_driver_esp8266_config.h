/*
 * bsp_driver_esp8266_config.h
 *
 *  Created on: Oct 21, 2019
 *      Author: harganda
 */

#ifndef __BSP_DRIVER_ESP8266_CONFIG_H_
#define __BSP_DRIVER_ESP8266_CONFIG_H_

#include "stm32f746xx.h"
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_uart.h"

#define ESP8266_DEF_UART_HANDLER   USART6
#define ESP8266_DEF_UART_BAUDRATE  115200

#define ESP8266_DEF_RST_GPIO_PORT  GPIOA
#define ESP8266_DEF_RST_GPIO_PIN   GPIO_PIN_10

#define ESP8266_DEF_EN_GPIO_PORT   GPIOA
#define ESP8266_DEF_EN_GPIO_PIN    GPIO_PIN_11

#endif /* __BSP_DRIVER_ESP8266_CONFIG_H_ */
