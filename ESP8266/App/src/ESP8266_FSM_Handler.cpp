/*
 * ESP8266_FSM_Handler.cpp
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#include "stm32_cli.h"
#include "cmsis_os.h"

#include "ESP8266_FSM_Handler.h"
#include "ESP8266_Utils.h"
#include "ESP8266_AT_Parser.h"

#include "ESP8266_timeout.h"
#include "ESP8266_timeout.h"

/* State handlers */
#include "ESP8266_handlers_common.h"
#include "ESP8266_handlers_wifi.h"
#include "ESP8266_handlers_tcp.h"
#include "ESP8266_handlers_ntp.h"

#ifdef __STM32_CLI_H_
	#define DBG_ESP(...) do{BSP_CLI_Printf("[ESP8266] %s --> ", __FUNCTION__); BSP_CLI_Printf(__VA_ARGS__);} while(0)
#else
	#define DBG_ESP(...)
#endif

typedef enum
{
    ST_STEP_INIT = 0,
    ST_STEP_SEND_CMD,
    ST_STEP_HANDLE,
} esp_fsm_step_t;

typedef struct
{
	esp_fsm_step_t step;
    void (*initializer)(esp_fsm_scratchpad_t*);
    void (*handler)(esp_fsm_scratchpad_t*, esp_event_data_t*);
    void (*builder)(esp_fsm_scratchpad_t*, const char*);
    const char* cmd;
    uint32_t timeout;
} esp_fsm_state_handler_t;

#if ESP8266_FSM_DEBUG == 1
#define ESP8266_FSM_STATE_X(id, desc, init, handler, builder, cmd, tout) desc,
const char* states_description[] = {
    ESP8266_FSM_STATES_TABLE_X
};
#undef ESP8266_FSM_STATE_X
#endif

#define ESP8266_FSM_STATE_X(id, desc, init, handler, builder, cmd, tout) {ST_STEP_INIT, init, handler, builder, cmd, tout},
static esp_fsm_state_handler_t esp_fsm_states[] = {
    ESP8266_FSM_STATES_TABLE_X
};
#undef ESP8266_FSM_STATE_X

osPoolId  eventPoolId;
osMessageQId eventQueueID;
osThreadId espTaskID;
osTimerId espTimerID;

static void ESP8266_Task(void * argument);
static void ESP8266_Timer_Cb(void const *argument);

static esp_fsm_scratchpad_t esp_fsm_scratchpad;

static const char* esp_states_description(esp_fsm_state_t idx);

static const char* esp_states_description(esp_fsm_state_t idx)
{
#if ESP8266_FSM_DEBUG == 1
    return states_description[idx];
#else
    return NULL;
#endif
}

void ESP8266_Init(void)
{
    uint8_t i = 0;

    /* Set scratchpad to default values */
    esp_fsm_scratchpad.prev_st = ST_INVALID;
    esp_fsm_scratchpad.curr_st = ST_INVALID;

    /* Reset scratchpad parameters */
    memset(esp_fsm_scratchpad.server, 0x00, sizeof(esp_fsm_scratchpad.server));
    memset(esp_fsm_scratchpad.path, 0x00, sizeof(esp_fsm_scratchpad.path));
    memset(esp_fsm_scratchpad.token, 0x00, sizeof(esp_fsm_scratchpad.token));
    memset(esp_fsm_scratchpad.ap_ssid, 0x00, sizeof(esp_fsm_scratchpad.ap_ssid));
    memset(esp_fsm_scratchpad.ap_pass, 0x00, sizeof(esp_fsm_scratchpad.ap_pass));
    esp_fsm_scratchpad.port = 0;
    esp_fsm_scratchpad.ap_update_needed = 0;

    /* Set all states to default status */
    for(i = 0; i < sizeof(esp_fsm_states)/sizeof(esp_fsm_state_handler_t); i++)
    {
        esp_fsm_states[i].step = ST_STEP_INIT;
    }

    if (eventPoolId == NULL)
    {
    	osPoolDef(eventPoolId, EVENT_QUEUE_LEN, esp_event_data_t);
    	eventPoolId = osPoolCreate(osPool(eventPoolId));
    }

    if (eventQueueID == NULL)
	{
    	osMessageQDef(eventQueueID, EVENT_QUEUE_LEN, esp_event_data_t);
    	eventQueueID = osMessageCreate(osMessageQ(eventQueueID), NULL);
	}

    if (espTaskID == NULL)
	{
		/* Create Task */
		osThreadDef(ESP_Task, ESP8266_Task, osPriorityNormal, 0, (configMINIMAL_STACK_SIZE * 4));
		espTaskID = osThreadCreate(osThread(ESP_Task), NULL);
	}

    if (espTimerID == NULL)
    {
    	osTimerDef(espTimerID, ESP8266_Timer_Cb);
    	espTimerID = osTimerCreate(osTimer(espTimerID), osTimerOnce, NULL);
    }

}

static void ESP8266_Task(void * argument)
{
	uint32_t PreviousWakeTime;
	esp_event_data_t ev_data;
	osEvent event;

    /* Do initial transition */
    esp_fsm_state_transition(ST_HARD_RST);

	/* Infinite loop */
	for(;;)
	{
		switch(esp_fsm_states[esp_fsm_scratchpad.curr_st].step)
		{

		/* Initialization */
		case ST_STEP_INIT:
		{
			if (NULL != esp_fsm_states[esp_fsm_scratchpad.curr_st].initializer)
			{
				esp_fsm_states[esp_fsm_scratchpad.curr_st].initializer(&esp_fsm_scratchpad);
			}

			/* Next step: Send Command */
			esp_fsm_step_transition(ST_STEP_SEND_CMD);
		}
		break;

		/* Send Command */
		case ST_STEP_SEND_CMD:
		{
			/* If builder was provided, execute command */
			if (NULL != esp_fsm_states[esp_fsm_scratchpad.curr_st].builder)
			{
				esp_fsm_states[esp_fsm_scratchpad.curr_st].builder(&esp_fsm_scratchpad, esp_fsm_states[esp_fsm_scratchpad.curr_st].cmd);
			}

			/* State Timeout setup */
			if(0 < esp_fsm_states[esp_fsm_scratchpad.curr_st].timeout)
			{
				osTimerStart(espTimerID, esp_fsm_states[esp_fsm_scratchpad.curr_st].timeout);
			}
			else
			{
				osTimerStop(espTimerID);
			}

			/* Next step: Handle state */
			esp_fsm_step_transition(ST_STEP_HANDLE);
		}
		break;

		/* Handle command */
		case ST_STEP_HANDLE:
		{
			osEvent evt = osMessageGet(eventQueueID, 0);
			if (evt.status == osEventMessage)
			{
				esp_event_data_t *pEventData = (esp_event_data_t*)evt.value.p;

				if (NULL != esp_fsm_states[esp_fsm_scratchpad.curr_st].handler)
				{
					esp_fsm_states[esp_fsm_scratchpad.curr_st].handler(&esp_fsm_scratchpad, pEventData);
				}

				ESP8266_Execute_Cb(&esp_fsm_scratchpad, pEventData);

				osPoolFree(eventPoolId, pEventData);
			}
			else if((NULL != esp_fsm_states[esp_fsm_scratchpad.curr_st].handler) && (ST_FSM_IDLE == esp_fsm_scratchpad.curr_st))
			{
				esp_fsm_states[esp_fsm_scratchpad.curr_st].handler(&esp_fsm_scratchpad, NULL);
			}
		}
		break;
		default:
			break;

		}

		osDelay(1);
	}
}

typedef void (*ESP8266_Callback)(uint8_t status, esp_event_data_t* ev_ptr);

void Dummy_Callback(uint8_t status, esp_event_data_t* ev_ptr)
{
	return 0;
}

static void ESP8266_Execute_Cb(esp_fsm_scratchpad_t* sp, esp_event_data_t* ev_ptr)
{
	/* TODO: Remove dummy events and add function to register callbacks */
	esp_fsm_state_t registered_state = ST_TCP_SEND;
	esp_event_t registered_event = AT_EV_OK;
	ESP8266_Callback myCB = Dummy_Callback;

	if (sp->curr_st == registered_state)
	{
		if (ev_ptr->ev == registered_event)
		{
			myCB(1, ev_ptr);
		}
		else if ((ev_ptr->ev == AT_EV_TIMEOUT) || (ev_ptr->ev == AT_EV_ERROR))
		{
			myCB(0, ev_ptr);
		}
	}
}


static void ESP8266_Timer_Cb(void const *argument)
{
	esp_event_data_t *message = (esp_event_data_t*)osPoolAlloc(eventPoolId);
	message->ev = AT_EV_TIMEOUT;
	if (osOK != osMessagePut(eventQueueID, (uint32_t)message, osWaitForever))
	{
		osPoolFree(eventPoolId, message);
	}
}

void esp_fsm_step_transition(esp_fsm_step_t step)
{
	DBG_ESP("Step from %d to %d\r\n", esp_fsm_states[esp_fsm_scratchpad.curr_st].step, step);
	esp_fsm_states[esp_fsm_scratchpad.curr_st].step = step;
}

void esp_fsm_state_transition(esp_fsm_state_t state)
{
	DBG_ESP("State from %d to %d\r\n", esp_fsm_scratchpad.curr_st, state);

    /* Reset current state steps */
    esp_fsm_states[esp_fsm_scratchpad.curr_st].step = ST_STEP_INIT;

    /* Set current state */
    esp_fsm_scratchpad.prev_st = esp_fsm_scratchpad.curr_st;
    esp_fsm_scratchpad.curr_st = state;

    /* Reset retry counter */
    esp_fsm_scratchpad.retryCounter = 3;
    /*
     * NOTE: On every state transition clear the buffers so we can receive only fresh data
     */

    /* Reset AT Parser */
    esp_parser_reset();
}

void esp_fsm_error_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev)
{

#if AT_EV_QUEUE_DEBUG == 1
	DBG_ESP("[ESP-ERR] Event: %s\r\n", esp_events_description(ev->ev));
#else
	DBG_ESP("[ESP-ERR] Event: %d\r\n", ev->ev);
#endif

#if ESP8266_FSM_DEBUG == 1
	DBG_ESP("[ESP-ERR] State: [%s]\r\n", esp_states_description(fsm_ptr->curr_st));
#else
	DBG_ESP("[ESP-ERR] State: [%d]\r\n", fsm_ptr->curr_st);
#endif

    if(0 < esp_fsm_scratchpad.retryCounter)
    {
        esp_fsm_scratchpad.retryCounter--;
        DBG_ESP("Retries left (%d)", esp_fsm_scratchpad.retryCounter);

        /* Send command again */
        esp_fsm_step_transition(ST_STEP_SEND_CMD);
    }
    else
    {
        /* Do initial transition */
        esp_fsm_state_transition(ST_HARD_RST);
    }
}

void esp_fsm_unexpected_event(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev)
{
#if AT_EV_QUEUE_DEBUG == 1
	DBG_ESP("[ESP-UXP] Event: %s", esp_events_description(ev->ev));
#else
	DBG_ESP("[ESP-UXP] Event: %d", ev->ev);
#endif

#if ESP8266_FSM_DEBUG == 1
	DBG_ESP("[ESP-UXP] State: [%s]", esp_states_description(fsm_ptr->curr_st));
#else
	DBG_ESP("[ESP-UXP] State: [%d]", fsm_ptr->curr_st);
#endif
}
