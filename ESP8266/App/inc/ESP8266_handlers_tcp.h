/*
 * ESP8266_handlers_common.h
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_HANDLERS_TCP_H_
#define ESP8266_HANDLERS_TCP_H_

#include "ESP8266_FSM_Events.h"
#include "ESP8266_FSM_Handler.h"
#include "../config.h"

#define ESP8266_TCP_SEND_DBG      (0)

#define ESP8266_TCP_HARDCODED     (0)

/* TODO: Reduce buffer size */
#if (SENS_ENABLE_COMPENSATION == 0)
#define ESP8266_TCP_SEND_BUFF_LEN (256)
#else
#define ESP8266_TCP_SEND_BUFF_LEN (384)
#endif

#define TCP_TIME_SERVER             "www.google.com"
#define TCP_TIME_PORT               80


/* Initializers */
void esp_tcp_start_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_tcp_start_status_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_tcp_send_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_tcp_close_status_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_tcp_close_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_tcp_req_time_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_tcp_req_time_send_initializer(esp_fsm_scratchpad_t* fsm_ptr);

/* Builders */
void tcp_start_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);
void tcp_send_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);
void tcp_req_time_start(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);
void tcp_req_time_send_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);

/* Handlers */
void esp_tcp_setup_mux_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_start_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_start_status_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_send_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_send_wait_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_close_status_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_close_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_receive_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_req_time_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_req_time_send_wait_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_req_time_send_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_tcp_req_time_recv_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);


#endif /* ESP8266_HANDLERS_TCP_H_ */
