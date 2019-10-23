/*
 * ESP8266_handlers_ntp.h
 *
 *  Created on: 10/01/2017
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_HANDLERS_NTP_H_
#define ESP8266_HANDLERS_NTP_H_

#include "ESP8266_FSM_Events.h"
#include "ESP8266_FSM_Handler.h"

#define ESP8266_NTP_SEND_DBG (0)

/* time.nist.gov NTP server */
#define NTP_SERVER_NAME     "time.nist.gov"
#define NTP_PORT_REQUEST    123
#define NTP_PACKET_SIZE     48
#define NTP_LOCAL_PORT      8000
#define NTP_MODE            0

/* Initializers */
void esp_ntp_start_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_ntp_send_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_ntp_close_initializer(esp_fsm_scratchpad_t* fsm_ptr);

/* Builders */
void ntp_start_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);
void ntp_send_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);

/* Handlers */
void esp_ntp_start_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_ntp_send_wait_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_ntp_send_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_ntp_close_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_ntp_receive_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);

#endif /* ESP8266_HANDLERS_NTP_H_ */
