/*
 * ESP8266_handlers_wifi.h
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_HANDLERS_WIFI_H_
#define ESP8266_HANDLERS_WIFI_H_

#include "ESP8266_FSM_Events.h"
#include "ESP8266_FSM_Handler.h"

/* Initializers */
void esp_wifi_list_ap_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_wifi_mac_and_ipv4_initializer(esp_fsm_scratchpad_t* fsm_ptr);
void esp_wifi_join_ap_initializer(esp_fsm_scratchpad_t* fsm_ptr);

/* Builders */
void cwjap_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char*);

/* Handlers */
void esp_setup_wifi_autoconn_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_setup_wifi_mode_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_setup_ap_list_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_setup_dhcp_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_wifi_join_ap_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_wifi_quit_ap_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_wifi_list_ap_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);
void esp_wifi_mac_and_ipv4_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr);

#endif /* ESP8266_HANDLERS_WIFI_H_ */
