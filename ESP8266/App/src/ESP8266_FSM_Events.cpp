/*
 * ESP8266_FSM_Events.cpp
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#include "ESP8266_FSM_Events.h"
#include "ESP8266_FSM_config.h"
#include "cmsis_os.h"

#if AT_EV_QUEUE_DEBUG == 1
#define ESP8266_EVENT_ELEMENT_X(x, y) y,
static const char* events_description[] = {
    ESP8266_EVENTS_TABLE_X
};
#undef ESP8266_EVENT_ELEMENT_X
#endif


#if AT_EV_QUEUE_DEBUG == 1
const char* esp_events_description(esp_event_t idx)
{
    return events_description[idx];
}
#endif
