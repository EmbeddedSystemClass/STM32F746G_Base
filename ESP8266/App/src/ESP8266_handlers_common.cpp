/*
 * ESP8266_handlers_common.cpp
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#include "ESP8266_handlers_common.h"
#include "ESP8266_AT_CMD.h"
#include "ESP8266_Utils.h"
#include "bsp_driver_esp8266.h"

/* Private declarations */
at_common_message_order_t get_version_msg_order = AT_WAITING_FOR_PAYLOAD;

void generic_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    esp_printf((char*)at_cmd, AT_CMD_EOL);
}

void hard_reset_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
	BSP_ESP8266_Reset();
}

void esp_idle_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    if(NULL != ev_data_ptr)
    {
        switch (ev_data_ptr->ev)
        {
        case (AT_EV_ERROR):
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
            break;
        default:
            esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
        }
    }
    else if (0)
    {
    	/* Here external commands shall be called */
    	/* TODO: Implement upper layer requests */
    }

#if 0
    /* Update RTC via NTP */
    else if ((rtcUpdateTimerExpired(fsm_ptr)) && (WIFI_STATUS_CONNECTED == esp_wifi_status()))
    {
        fsm_ptr->rtc_last_update = unixEpoch();
#if NTP_SYNC
        /* Update RTC via NTP */
        esp_fsm_state_transition(ST_NTP_START);
#else
        /* Update RTC via TCP */
        esp_fsm_state_transition(ST_TCP_REQUEST_TIME);
#endif
    }
    /* Update Local IP */
    else if ((ipUpdateTimerExpired(fsm_ptr)) && (WIFI_STATUS_CONNECTED == esp_wifi_status()))
    {
        fsm_ptr->ip_last_update = unixEpoch();
        esp_fsm_state_transition(ST_WIFI_GET_IPV4_MAC);
    }
    /* Join Access Point */
    else if ((apjoinTimerExpired(fsm_ptr)) && (WIFI_STATUS_DISCONNECTED == esp_wifi_status()))
    {
        esp_fsm_state_transition(ST_WIFI_AP_JOIN);
    }
    else if (fsm_ptr->update_ap_list)
    {
        esp_update_ap_list(false);
        esp_fsm_state_transition(ST_WIFI_SHOW_AP_LIST);
    }
    /* Join Access Point on demand */
    else if (1 == fsm_ptr->ap_update_needed)
    {
        esp_fsm_state_transition(ST_WIFI_AP_JOIN);
    }
    /* Send report if logs are pending */
    else if((0 < get_logs_pending()) && (WIFI_STATUS_CONNECTED == esp_wifi_status()))
    {
        /* Get last log index */
        fsm_ptr->log_idx = get_logs_total() - 1;
        dprintfln("[LOG] Last pending index: %d", fsm_ptr->log_idx);

        /* Read log info */
        if(readLog(&fsm_ptr->data, fsm_ptr->log_idx))
        {
            /* If last log is not sent already, send it now */
            if(!fsm_ptr->data.sent)
            {
                /* LIFO behavior */
                dprintln("[TCP] Send log [LIFO]");
                strcpy(fsm_ptr->path, "/api/data/");
                esp_fsm_state_transition(ST_TCP_START);
            }
            else if(backlogTimerExpired(fsm_ptr))
            {
                fsm_ptr->log_idx = get_next_log_pending_index();
                if(readLog(&fsm_ptr->data, fsm_ptr->log_idx))
                {
                    /* FIFO behavior */
                    dprintln("[TCP] Send log [FIFO]");
                    strcpy(fsm_ptr->path, "/api/bulkdata/");
                    esp_fsm_state_transition(ST_TCP_START);
                    fsm_ptr->last_backlog_sent = unixEpoch();
                }
                else
                {
                    dprintfln("could not read log");
                }
            }
            else
            {
                /* Print to validate RTC count */
                dprintfln("Current epoch: %d", unixEpoch());
            }
        }
        else
        {
            dprintfln("could not read log");
        }
    }
#endif
}

void esp_test_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_DISABLE_ECHO);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_reset_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_READY):
        esp_fsm_state_transition(ST_TEST);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_disable_echo_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_GET_VERSION);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_setup_conn_mode_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_GET_VERSION);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_get_version_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    get_version_msg_order = AT_WAITING_FOR_PAYLOAD;
}

void esp_get_version_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_VERSION):
        if(AT_WAITING_FOR_PAYLOAD == get_version_msg_order)
        {
            /* TODO: Print and store received version */
            // version_data_t version = *(*version_data_t)(&ev_data_ptr->data);
            get_version_msg_order = AT_WAITING_FOR_OK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if(AT_WAITING_FOR_OK == get_version_msg_order)
        {
            get_version_msg_order = AT_WAITING_FINISHED;
            esp_fsm_state_transition(ST_WIFI_SETUP_MODE);
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

