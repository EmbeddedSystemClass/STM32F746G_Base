/*
 * stm32_cli.c
 *
 *  Created on: Oct 19, 2019
 *      Author: harganda
 */

#include "bsp_driver_esp8266.h"
#include "bsp_driver_esp8266_config.h"

#if (configBSP_ESP8266_ENABLE_TASK == 1)
#include "cmsis_os.h"
#endif

#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef* cliHandler = NULL;

#if (configBSP_ESP8266_ENABLE_TASK == 1)
static osMessageQId espQueueID;
static osThreadId espTaskID;
static void BSP_ESP8266_Task(void * argument);
#endif

static void BSP_ESP8266_GPIO_Init(GPIO_TypeDef  *GPIO_Port, uint32_t GPIO_Pin, GPIO_PinState GPIO_PinState);
static void BSP_ESP8266_GPIO_Write(GPIO_TypeDef  *GPIO_Port, uint32_t GPIO_Pin, GPIO_PinState GPIO_PinState);

static void BSP_ESP8266_GPIO_Init(GPIO_TypeDef  *GPIO_Port, uint32_t GPIO_Pin, GPIO_PinState GPIO_PinState)
{
	GPIO_InitTypeDef  xInitStruct;

	/* Configure the pin */
	xInitStruct.Pin = GPIO_Pin;
	xInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	xInitStruct.Pull = GPIO_PULLUP;
	xInitStruct.Speed = GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIO_Port, &xInitStruct);

	/* Set default state */
	BSP_ESP8266_GPIO_Write(GPIO_Port, GPIO_Pin, GPIO_PinState);
}

static void BSP_ESP8266_GPIO_Write(GPIO_TypeDef  *GPIO_Port, uint32_t GPIO_Pin, GPIO_PinState GPIO_PinState)
{
	HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PinState);
}

uint8_t BSP_ESP8266_Init(UART_HandleTypeDef* pxUARTHandler)
{
	uint8_t xReturn = 0;

    if (pxUARTHandler != NULL)
	{
    	cliHandler = pxUARTHandler;

		if (HAL_UART_STATE_READY == cliHandler->gState)
		{
			/* If UART Handler has been already initialized do nothing */
		}
		else
		{
			cliHandler->Instance = ESP8266_DEF_UART_HANDLER;
			cliHandler->Init.BaudRate = ESP8266_DEF_UART_BAUDRATE;
			cliHandler->Init.WordLength = UART_WORDLENGTH_8B;
			cliHandler->Init.StopBits = UART_STOPBITS_1;
			cliHandler->Init.Parity = UART_PARITY_NONE;
			cliHandler->Init.Mode = UART_MODE_TX_RX;
			cliHandler->Init.HwFlowCtl = UART_HWCONTROL_NONE;
			cliHandler->Init.OverSampling = UART_OVERSAMPLING_16;
			cliHandler->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
			cliHandler->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
			if (HAL_UART_Init(pxUARTHandler) != HAL_OK)
			{
				// Error_Handler();
			}
		}

#if (configBSP_ESP8266_ENABLE_TASK == 1)

	    if (espQueueID == NULL)
		{
			/* Create Queue */
			osMessageQDef(ESP8266_Queue, BSP_ESP8266_QUEUE_LEN, char*);
			espQueueID = osMessageCreate (osMessageQ(ESP8266_Queue), NULL);
		}

	    if (espTaskID == NULL)
	    {
	    	/* Create Task */
			osThreadDef(ESP8266_Task, BSP_ESP8266_Task, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
			espTaskID = osThreadCreate(osThread(ESP8266_Task), NULL);
	    }

	    if (espTaskID != NULL)
	    {
	    	BSP_ESP8266_GPIO_Init(ESP8266_DEF_RST_GPIO_PORT, ESP8266_DEF_RST_GPIO_PIN, GPIO_PIN_SET);
	    	BSP_ESP8266_GPIO_Init(ESP8266_DEF_EN_GPIO_PORT, ESP8266_DEF_EN_GPIO_PIN, GPIO_PIN_SET);

	    	xReturn = 1;
	    }
	    else
	    {
			/* Could not create the task, so delete the queue again. */
	    	osMessageDelete( espQueueID );
	    }

#else
		xReturn = 1;
#endif

	}

    return xReturn;
}

void BSP_ESP8266_Reset(void)
{
	BSP_ESP8266_GPIO_Write(ESP8266_DEF_RST_GPIO_PORT, ESP8266_DEF_RST_GPIO_PIN, GPIO_PIN_RESET);
	BSP_ESP8266_GPIO_Write(ESP8266_DEF_EN_GPIO_PORT, ESP8266_DEF_EN_GPIO_PIN, GPIO_PIN_RESET);

	osDelay(100);

	BSP_ESP8266_GPIO_Write(ESP8266_DEF_RST_GPIO_PORT, ESP8266_DEF_RST_GPIO_PIN, GPIO_PIN_SET);
	BSP_ESP8266_GPIO_Write(ESP8266_DEF_EN_GPIO_PORT, ESP8266_DEF_EN_GPIO_PIN, GPIO_PIN_SET);
}

uint16_t BSP_ESP8266_Write(const char* src, uint16_t len)
{
  if ((src != NULL) && (len > 0) && (cliHandler != NULL))
  {
    HAL_UART_Transmit(cliHandler, (uint8_t*)src, len, 100);
  }
  return len;
}

uint16_t BSP_ESP8266_Read(const char* dst, uint16_t len)
{
  return len;
}

#if (configBSP_ESP8266_ENABLE_TASK == 1)

uint16_t BSP_ESP8266_Printf(const char * fmt, ...)
{
  size_t xLength = 0;
  va_list args;
  void * pcPrintString = NULL;

  /* Allocate a buffer to hold the log message. */
  pcPrintString = pvPortMalloc(BSP_ESP8266_BUFFER_LEN);

  if(pcPrintString != NULL)
  {
    /* There are a variable number of parameters. */
    va_start( args, fmt );
    xLength = vsnprintf(pcPrintString, BSP_ESP8266_BUFFER_LEN, fmt, args);
    va_end( args );

    /* Only send the buffer to the logging task if it is not empty. */
    if(xLength > 0)
    {
        /* Send the string to the logging task for IO. */
    	if (osMessagePut(espQueueID, (uint32_t)pcPrintString, osWaitForever) != osOK)
    	{
            /* The buffer was not sent so must be freed again. */
            vPortFree(pcPrintString );
        }
    }
    else
    {
      /* The buffer was not sent, so it must be freed. */
      vPortFree(pcPrintString);
    }
  }

  return xLength;
}
#else

static char cliBuffer[BSP_ESP8266_BUFFER_LEN];

uint16_t BSP_ESP8266_Printf(const char* fmt, ...)
{
  uint16_t len = 0;
  va_list args;
  va_start(args, fmt);
  vsnprintf(cliBuffer, BSP_ESP8266_BUFFER_LEN, fmt, args);
  len = BSP_ESP8266_Write(&cliBuffer[0], strlen(cliBuffer));
  va_end(args);
  return len;
}

#endif

#if (configBSP_ESP8266_ENABLE_TASK == 1)

static void BSP_ESP8266_Task(void * argument)
{
	uint32_t PreviousWakeTime;
	osEvent event;

	/* Initialize the PreviousWakeTime variable with the current time. */
	PreviousWakeTime = osKernelSysTick();

	/* Infinite loop */
	for(;;)
	{
		/* Get the message from the queue */
		event = osMessageGet(espQueueID, osWaitForever);
		if (event.status == osEventMessage)
		{
			BSP_ESP8266_Write((char*)event.value.v, strlen((char*)event.value.v));
			vPortFree((void*)event.value.v);
		}

		osDelayUntil( &PreviousWakeTime, 10);
	}
}

#endif
