/*
 * ESP8266_handlers_common.h
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_HANDLERS_COMMON_H_
#define ESP8266_HANDLERS_COMMON_H_

#include "ESP8266_FSM_Events.h"
#include "ESP8266_FSM_Handler.h"

/* Initializers */
void esp_get_version_initializer(esp_fsm_scratchpad_t* fsm_ptr);

/* Builders */
void generic_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);
void hard_reset_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd);

/* Handlers */
void esp_idle_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_test_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_reset_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_disable_echo_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_setup_conn_mode_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_get_version_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);

#endif /* ESP8266_HANDLERS_COMMON_H_ */
