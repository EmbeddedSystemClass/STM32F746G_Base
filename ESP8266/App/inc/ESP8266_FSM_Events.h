/*
 * ESP8266_AT_Parser.h
 *
 *  Created on: 16/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_AT_EVENTS_H_
#define ESP8266_AT_EVENTS_H_

#include <Arduino.h>
#include "ESP8266_FSM_config.h"

#define ESP8266_EVENTS_TABLE_X \
    /* --------------------- Event Identifier, -------- "012345678901234" -- */ \
    ESP8266_EVENT_ELEMENT_X( AT_EV_INVALID,             "INVALID"    )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_DUMMY,               "DUMMY"      )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_OK,                  "OK"         )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_ERROR,               "ERROR"      )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_FAIL,                "FAIL"       )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_BUSY,                "BUSY"       )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TIMEOUT,             "TIMEOUT"    )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_READY,               "READY"      )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_VERSION,             "VERSION"    )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_WIFI_DISCONNECT,     "WIFI_DSC"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_WIFI_CWJAP,          "CWJAP_ERR"  )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_WIFI_CWJAP_OK,       "CWJAP_OK"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_WIFI_CWJAP_IP,       "CWJAP_IP"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_WIFI_CWLAP,          "CWLAP"      )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_STATUS,          "TCP_STAT"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_CONNECT,         "TCP_CONN"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_ALREADY_CONNECT, "TCP_ALRD"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_CLOSED,          "TCP_CLOSE"  )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_CURSOR,          "TCP_CRSR"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_SENDOK,          "TCP_SOK"    )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_SEND_ERROR,      "TCP_SERR"   )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TCP_IPD,             "IPD"        )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_LOCAL_IP,            "IPv4"       )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_LOCAL_MAC,           "MAC"        )          \
    ESP8266_EVENT_ELEMENT_X( AT_EV_TIME,                "TIME"       )          \


#define ESP8266_EVENT_ELEMENT_X(x, y) x,
typedef enum
{
    ESP8266_EVENTS_TABLE_X
} esp_event_t;
#undef ESP8266_EVENT_ELEMENT_X

typedef struct
{
    esp_event_t ev;
    uint8_t data[8];
} esp_event_data_t;

typedef enum
{
    AT_WAITING_FOR_PAYLOAD,
    AT_WAITING_FOR_OK,
    AT_WAITING_FINISHED,
} at_common_message_order_t;

#if AT_EV_QUEUE_DEBUG == 1
const char* esp_events_description(esp_event_t idx);
#endif

#endif /* ESP8266_AT_EVENTS_H_ */

