/*
 * ESP8266_handlers_wifi.cpp
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#include "ESP8266_handlers_wifi.h"
#include "ESP8266_AT_CMD.h"
#include "ESP8266_Utils.h"

typedef enum
{
    AT_WAITING_FOR_STAIP,
    AT_WAITING_FOR_STAMAC,
    AT_WAITING_FOR_STAOK,
    AT_WAITING_FOR_STA_FINISHED
}at_cifsr_message_order_t;

at_common_message_order_t wifi_join_msg_order = AT_WAITING_FOR_PAYLOAD;
at_cifsr_message_order_t wifi_cifsr_msg_order = AT_WAITING_FOR_STAIP;

void esp_setup_wifi_autoconn_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_WIFI_SETUP_MODE);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_setup_wifi_mode_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_WIFI_SETUP_AP_LIST);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_setup_ap_list_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_WIFI_SETUP_DHCP);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_setup_dhcp_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_WIFI_AP_QUIT);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void cwjap_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    char* ssid_ptr = NULL;
    char* pass_ptr = NULL;

    if (NULL == at_cmd)
    {
        if(1 == fsm_ptr->ap_update_needed)
        {
            ssid_ptr = fsm_ptr->ap_new_ssid;
            pass_ptr = fsm_ptr->ap_new_pass;
        }
        else
        {
            ssid_ptr = fsm_ptr->ap_ssid;
            pass_ptr = fsm_ptr->ap_pass;
        }

        dprintfln("[ESP] Connecting to AP: \"%s\"", ssid_ptr);

        esp_printf(AT_CMD_WIFI_AP_JOIN, ssid_ptr, pass_ptr, AT_CMD_EOL);
    }
}

void esp_wifi_join_ap_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    wifi_join_msg_order = AT_WAITING_FOR_PAYLOAD;
}

void esp_wifi_join_ap_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):

        /* Mark operation as successful */
        esp_update_wifi_status(WIFI_STATUS_CONNECTED);

        /* If connection was requested and successfully executed, save SSID and PASS */
        if(1 == fsm_ptr->ap_update_needed)
        {
            /* Copy to scratchpad */
            strcpy((char*)&fsm_ptr->ap_ssid, fsm_ptr->ap_new_ssid);
            strcpy((char*)&fsm_ptr->ap_pass, fsm_ptr->ap_new_pass);

            /* Clear flag */
            fsm_ptr->ap_update_needed = 0;
        }
        fsm_ptr->ap_last_update = unixEpoch();
        esp_fsm_state_transition(ST_WIFI_GET_IPV4_MAC);
        break;
    case (AT_EV_FAIL):
        esp_update_wifi_status(WIFI_STATUS_DISCONNECTED);
        fsm_ptr->ap_update_needed = 0;
        fsm_ptr->ap_last_update = unixEpoch();
        esp_fsm_state_transition(ST_FSM_IDLE);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_update_wifi_status(WIFI_STATUS_DISCONNECTED);
        fsm_ptr->ap_last_update = unixEpoch();
        fsm_ptr->ap_update_needed = 0;
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_wifi_quit_ap_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
    case (AT_EV_ERROR):
        esp_fsm_state_transition(ST_WIFI_AP_JOIN);
        break;
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}
void esp_wifi_list_ap_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    uint8_t i;
    for (i = 0; i < (sizeof(fsm_ptr->ap_ssid_addresses) / sizeof(fsm_ptr->ap_ssid_addresses[0])) ; i++)
    {
        fsm_ptr->ap_ssid_addresses[i] = 0;
    }
    /* Variable to count how many SSIDs were received */
    fsm_ptr->ap_ssid_list_count = 0;
    fsm_ptr->ap_list_updated_finished = false;
}

void esp_wifi_list_ap_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_WIFI_CWLAP):
        {
            if (ESP8266_MAX_SSID_BUFF > fsm_ptr->ap_ssid_list_count)
            {
                fsm_ptr->ap_ssid_addresses[fsm_ptr->ap_ssid_list_count++] = *(uint32_t *)(&ev_data_ptr->data[0]);
            }
        }
        break;
    case (AT_EV_OK):
        fsm_ptr->ap_list_updated_finished = true;
        esp_fsm_state_transition(ST_FSM_IDLE);
        break;
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_wifi_mac_and_ipv4_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    wifi_cifsr_msg_order = AT_WAITING_FOR_STAIP;
}

void esp_wifi_mac_and_ipv4_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_LOCAL_IP):
        if(AT_WAITING_FOR_STAIP == wifi_cifsr_msg_order)
        {
            /* Copy IPv4 data from event */
            dprintfln("IP = %d.%d.%d.%d", ev_data_ptr->data[0], ev_data_ptr->data[1], ev_data_ptr->data[2], ev_data_ptr->data[3]);

            wifi_cifsr_msg_order = AT_WAITING_FOR_STAMAC;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_LOCAL_MAC):
        if (AT_WAITING_FOR_STAMAC == wifi_cifsr_msg_order)
        {
            /* Copy MAC data from event */
            memcpy(&fsm_ptr->mac[0], &ev_data_ptr->data[0], sizeof(fsm_ptr->mac));

            dprint("[ESP] MAC = ");
            printHex((uint8_t*)&fsm_ptr->mac[0], sizeof(fsm_ptr->mac));
            dprintln();

            wifi_cifsr_msg_order = AT_WAITING_FOR_STAOK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if (AT_WAITING_FOR_STAOK == wifi_cifsr_msg_order)
        {
            wifi_cifsr_msg_order = AT_WAITING_FOR_STA_FINISHED;
            fsm_ptr->ip_last_update = unixEpoch();
            esp_fsm_state_transition(ST_FSM_IDLE);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}
