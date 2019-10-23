/*
 * ESP8266_FSM_Handler.h
 *
 *  Created on: 16/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_FSM_HANDLER_H_
#define ESP8266_FSM_HANDLER_H_

#include "ESP8266_FSM_Events.h"
#include "ESP8266_AT_CMD.h"
#include "ESP8266_FSM_config.h"

#undef ESP8266_FSM_STATE_X

#define ESP8266_FSM_STATES_TABLE_X \
    /* ----------------- State Identifier, -------- "012345678901", -- State Initializer, ------------------- State Handler, ----------------      Command builder, ------------------- AT Command, ------------------- Timeout */ \
    ESP8266_FSM_STATE_X( ST_INVALID,                "INVALID",         NULL,                                  NULL,                                NULL,                                NULL,                           0u       ) \
    ESP8266_FSM_STATE_X( ST_FSM_IDLE,               "FSM_IDLE",        NULL,                                  esp_idle_handler,                    NULL,                                NULL,                           0u       ) \
    ESP8266_FSM_STATE_X( ST_HARD_RST,               "HARD_RST",        NULL,                                  esp_reset_handler,                   hard_reset_at_builder,               NULL,                           10000u   ) \
    ESP8266_FSM_STATE_X( ST_SOFT_RST,               "SOFT_RST",        NULL,                                  esp_reset_handler,                   generic_at_builder,                  AT_CMD_RST,                     10000u   ) \
    ESP8266_FSM_STATE_X( ST_TEST,                   "TEST",            NULL,                                  esp_test_handler,                    generic_at_builder,                  AT_CMD_TEST,                    1000u    ) \
    ESP8266_FSM_STATE_X( ST_DISABLE_ECHO,           "DISABLE_ECHO",    NULL,                                  esp_disable_echo_handler,            generic_at_builder,                  AT_CMD_DISABLE_ECHO,            1000u    ) \
    ESP8266_FSM_STATE_X( ST_GET_VERSION,            "GET_VERSION",     esp_get_version_initializer,           esp_get_version_handler,             generic_at_builder,                  AT_CMD_VERSION,                 10000u   ) \
    /* ----------------- State Identifier, -------- "012345678901", -- State Initializer, ------------------- State Handler, ----------------      Command builder, ------------------- AT Command, ------------------- Timeout */ \
    ESP8266_FSM_STATE_X( ST_WIFI_SETUP_AUTOCONN,    "WS_AUTOCONN",     NULL,                                  esp_setup_wifi_autoconn_handler,     generic_at_builder,                  AT_CMD_WIFI_AUTOCONN,           1000u    ) \
    ESP8266_FSM_STATE_X( ST_WIFI_SETUP_MODE,        "WS_MODE",         NULL,                                  esp_setup_wifi_mode_handler,         generic_at_builder,                  AT_CMD_WIFI_MODE,               1000u    ) \
    ESP8266_FSM_STATE_X( ST_WIFI_SETUP_AP_LIST,     "WS_APLIST",       NULL,                                  esp_setup_ap_list_handler,           generic_at_builder,                  AT_CMD_WIFI_AP_LIST_CFG,        1000u    ) \
    ESP8266_FSM_STATE_X( ST_WIFI_SETUP_DHCP,        "WS_DHCP",         NULL,                                  esp_setup_dhcp_handler,              generic_at_builder,                  AT_CMD_WIFI_DHCP,               1000u    ) \
    ESP8266_FSM_STATE_X( ST_WIFI_SHOW_AP_LIST,      "AP_SHOW",         esp_wifi_list_ap_initializer,          esp_wifi_list_ap_handler,            generic_at_builder,                  AT_CMD_WIFI_AP_LIST,            8000u    ) \
    ESP8266_FSM_STATE_X( ST_WIFI_AP_QUIT,           "AP_QUIT",         NULL,                                  esp_wifi_quit_ap_handler,            generic_at_builder,                  AT_CMD_WIFI_AP_QUIT,            5000u    ) \
    ESP8266_FSM_STATE_X( ST_WIFI_AP_JOIN,           "AP_JOIN",         esp_wifi_join_ap_initializer,          esp_wifi_join_ap_handler,            cwjap_at_builder,                    NULL,                           15000u   ) \
    ESP8266_FSM_STATE_X( ST_WIFI_GET_IPV4_MAC,      "IP&MAC",          esp_wifi_mac_and_ipv4_initializer,     esp_wifi_mac_and_ipv4_handler,       generic_at_builder,                  AT_CMD_GET_IP_MAC,              5000u    ) \
    /* ----------------- State Identifier, -------- "012345678901", -- State Initializer, ------------------- State Handler, ----------------      Command builder, ------------------- AT Command, ------------------- Timeout */ \
    ESP8266_FSM_STATE_X( ST_TCP_MUX,                "TCP_MUX",         NULL,                                  esp_tcp_setup_mux_handler,           generic_at_builder,                  AT_CMD_CIP_MUX,                 1000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_START,              "TCP_START",       esp_tcp_start_initializer,             esp_tcp_start_handler,               tcp_start_at_builder,                NULL,                           5000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_START_STAT,         "TCP_STR_STAT",    esp_tcp_start_status_initializer,      esp_tcp_start_status_handler,        generic_at_builder,                  AT_CMD_CIP_STATUS,              1000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_SEND,               "TCP_SEND",        esp_tcp_send_initializer,              esp_tcp_send_handler,                tcp_send_at_builder,                 NULL,                           5000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_SEND_WAIT,          "TCP_SEND_WAIT",   NULL,                                  esp_tcp_send_wait_handler,           NULL,                                NULL,                           1000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_CLOSE_STAT,         "TCP_CLS_STAT",    esp_tcp_close_status_initializer,      esp_tcp_close_status_handler,        generic_at_builder,                  AT_CMD_CIP_STATUS,              1000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_CLOSE,              "TCP_CLOSE",       esp_tcp_close_initializer,             esp_tcp_close_handler,               generic_at_builder,                  AT_CMD_TCP_CLOSE,               5000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_RECEIVE,            "TCP_RECEIVE",     NULL,                                  esp_tcp_receive_handler,             NULL,                                NULL,                           10000u   ) \
    /* ----------------- State Identifier, -------- "012345678901", -- State Initializer, ------------------- State Handler, ----------------      Command builder, ------------------- AT Command, ------------------- Timeout */ \
    ESP8266_FSM_STATE_X( ST_NTP_START,              "NTP_START",       esp_ntp_start_initializer,             esp_ntp_start_handler,               ntp_start_at_builder,                NULL,                           5000u    ) \
    ESP8266_FSM_STATE_X( ST_NTP_SEND,               "NTP_SEND",        esp_ntp_send_initializer,              esp_ntp_send_handler,                ntp_send_at_builder,                 NULL,                           5000u    ) \
    ESP8266_FSM_STATE_X( ST_NTP_SEND_WAIT,          "NTP_SEND_WAIT",   NULL,                                  esp_ntp_send_wait_handler,           NULL,                                NULL,                           1000u    ) \
    ESP8266_FSM_STATE_X( ST_NTP_CLOSE,              "NTP_CLOSE",       esp_ntp_close_initializer,             esp_ntp_close_handler,               generic_at_builder,                  AT_CMD_UDP_CLOSE,               5000u    ) \
    ESP8266_FSM_STATE_X( ST_NTP_RECEIVE,            "NTP_RECEIVE",     NULL,                                  esp_ntp_receive_handler,             NULL,                                NULL,                           20000u   ) \
    /* ----------------- State Identifier, -------- "012345678901", -- State Initializer, ------------------- State Handler, ----------------      Command builder, ------------------- AT Command, ------------------- Timeout */ \
    ESP8266_FSM_STATE_X( ST_TCP_REQUEST_TIME,       "TCP_REQ_TIME",    esp_tcp_req_time_initializer,          esp_tcp_req_time_handler,            tcp_req_time_start,                  NULL,                           5000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_REQ_TIME_SEND_WAIT, "TCP_REQ_SND_WAIT",NULL,                                  esp_tcp_req_time_send_wait_handler,  NULL,                                NULL,                           1000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_REQ_TIME_SEND,      "TCP_REQ_SEND",    esp_tcp_req_time_send_initializer,     esp_tcp_req_time_send_handler,       tcp_req_time_send_at_builder,        NULL,                           5000u    ) \
    ESP8266_FSM_STATE_X( ST_TCP_REQ_TIME_RECEIVE,   "TCP_REQ_RECV",    NULL,                                  esp_tcp_req_time_recv_handler,       NULL,                                NULL,                           5000u    ) \


#define ESP8266_FSM_STATE_X(id, desc, init, handler, builder, cmd, tout) id,
typedef enum
{
    ESP8266_FSM_STATES_TABLE_X
} esp_fsm_state_t;
#undef ESP8266_FSM_STATE_X

typedef enum
{
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
} wifi_status_t;

typedef struct
{
    esp_fsm_state_t curr_st;
    esp_fsm_state_t prev_st;
    uint8_t         retryCounter;
    /* Local data */
    uint8_t         ipv4[4];            /* IPv4 Address */
    uint8_t         mac[6];             /* MAC Address */
    /* AP Management vars */
    char            ap_ssid[ESP8266_SSID_LEN];
    char            ap_pass[ESP8266_SSID_LEN];
    bool            update_ap_list;
    uint8_t         ap_ssid_list_count;
    bool            ap_list_updated_finished;
    uint32_t        ap_ssid_addresses[ESP8266_MAX_SSID_BUFF];
    uint8_t         ap_update_needed;
    uint8_t         ap_connected;
    char*           ap_new_ssid;
    char*           ap_new_pass;
    /* Server information */
    char            server[32];
    char            path[16];
    char            token[32];
    uint16_t        port;               /* Sensor data log index */
} esp_fsm_scratchpad_t;

/* This function should be executed only once on start-up */
void ESP8266_Init(void);

/* This function should be executed periodically */
void esp_fsm_loop();

void esp_fsm_state_transition(esp_fsm_state_t state);

/* Handle FSM errors */
void esp_fsm_error_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev);

/* Handle unexpected events */
void esp_fsm_unexpected_event(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev);

#endif /* ESP8266_FSM_HANDLER_H_ */
