/*
 * ESP8266_handlers_ntp.cpp
 *
 *  Created on: 10/01/2017
 *      Author: Desarrollo 01
 */

#include "ESP8266_handlers_ntp.h"
#include "ESP8266_AT_CMD.h"
#include "ESP8266_AT_Parser.h"
#include "ESP8266_Utils.h"
#include "../sensorsHandler.h"
#include "../logHandler.h"
#include "../utils.h"
#include "../dprint.h"

#define TCP_SEND_FSM_DBG (0)

typedef enum
{
    BUILDER_SEND_NTP,                   /* +CIPSEND:<len> */
    BUILDER_SEND_NTP_DATA,              /* > POST /api/data/ HTTP/1.1 */
    BUILDER_FINISHED,
} at_ntp_send_message_order_t;

typedef enum
{
    AT_SEND_WAIT_FOR_OK,
    AT_SEND_WAIT_CURSOR,
    AT_SEND_WAIT_SENDOK,
    AT_SEND_FINISHED,
} at_ntp_send_builder_order_t;


typedef struct
{
        at_ntp_send_message_order_t order;
        char buffer[NTP_PACKET_SIZE];
        uint8_t len;
} ntp_send_scratchpad_t;

#if TCP_SEND_FSM_DBG == 1
static const char* sendLabels[]
{
    "AT_SEND_WAIT_FOR_OK",
    "AT_SEND_WAIT_CURSOR",
    "AT_SEND_WAIT_SENDOK",
    "AT_SEND_FINISHED",
};
#endif

/* Private declarations */
static at_ntp_send_builder_order_t ntp_send_msg_order = AT_SEND_WAIT_FOR_OK;
static at_common_message_order_t ntp_close_msg_order = AT_WAITING_FOR_PAYLOAD;

static ntp_send_scratchpad_t esp_ntp_scratchpad;

static void send_transition(at_ntp_send_builder_order_t state);

static void send_transition(at_ntp_send_builder_order_t state)
{
#if TCP_SEND_FSM_DBG == 1
    dprint("SEND-FSM: [");
    dprint(sendLabels[ntp_send_msg_order]);
    dprint("] -> [");
    dprint(sendLabels[state]);
    dprintln("]");
#endif
    ntp_send_msg_order = state;
}

void ntp_start_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    if (NULL == at_cmd)
    {
        esp_printf(AT_CMD_UDP_START, NTP_SERVER_NAME, NTP_PORT_REQUEST, NTP_LOCAL_PORT, AT_CMD_EOL);
    }
}

void ntp_send_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    uint32_t data_len;
    switch(esp_ntp_scratchpad.order)
    {
    case (BUILDER_SEND_NTP):

        /* Fill with zeroes */
        memset(&(esp_ntp_scratchpad.buffer[0]), 0, NTP_PACKET_SIZE);

        /* Initialize values needed to form NTP request */
        /* LI, Version, Mode */
        esp_ntp_scratchpad.buffer[0] = 0xE3; /* 0b1110 0011 */
        /* Stratum, or type of clock */
        esp_ntp_scratchpad.buffer[1] = 0x00;
        /* Polling Interval */
        esp_ntp_scratchpad.buffer[2] = 0x06;
        /* Peer Clock Precision */
        esp_ntp_scratchpad.buffer[3] = 0xEC;
        /* 8 bytes of zero for Root Delay & Root Dispersion */
        esp_ntp_scratchpad.buffer[12] = 0x31;
        esp_ntp_scratchpad.buffer[13] = 0x4E;
        esp_ntp_scratchpad.buffer[14] = 0x31;
        esp_ntp_scratchpad.buffer[15] = 0x34;

        /* Send Command */
        esp_printf(AT_CMD_UDP_SEND, NTP_PACKET_SIZE, AT_CMD_EOL);
        esp_ntp_scratchpad.order = BUILDER_SEND_NTP_DATA;
        break;
    case (BUILDER_SEND_NTP_DATA):

        /* Send payload */
        ESP8266_UART.write((char*)&(esp_ntp_scratchpad.buffer[0]), NTP_PACKET_SIZE);
        esp_ntp_scratchpad.order = BUILDER_FINISHED;
        break;
    default:
        /* Do nothing */
        break;
    }

#if ESP8266_NTP_SEND_DBG == 1
    /* Debug data to send */
    switch(esp_ntp_scratchpad.order)
    {
    case (BUILDER_SEND_NTP_DATA):
        dprint("### +CIPSEND: ");
        printHex((uint8_t*)&(esp_ntp_scratchpad.buffer[0]), NTP_PACKET_SIZE);
        dprintln();

        /* Clear event data field before writing to it */
        memset(esp_ntp_scratchpad.buffer, 0x00, 48);

        dprint("### +BUFFER: ");
        printHex((uint8_t*)&(esp_ntp_scratchpad.buffer[0]), NTP_PACKET_SIZE);
        dprintln();

        break;
    default:
        /* Do nothing */
        break;
    }
#endif

}

void esp_ntp_start_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    esp_ntp_scratchpad.order = BUILDER_SEND_NTP;
    esp_parser_ntp_buffer(&(esp_ntp_scratchpad.buffer[0]));
    dprintln(("[ESP] NTP Sync request"));
}

void esp_ntp_start_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_NTP_SEND_WAIT);
        break;
    case (AT_EV_TCP_CLOSED):
        /* Do nothing */
        break;
    case (AT_EV_ERROR):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    case (AT_EV_TIMEOUT):
        esp_fsm_state_transition(ST_FSM_IDLE);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_ntp_send_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    ntp_send_msg_order = AT_SEND_WAIT_FOR_OK;
}

void esp_ntp_send_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        if(AT_SEND_WAIT_FOR_OK == ntp_send_msg_order)
        {
            send_transition(AT_SEND_WAIT_CURSOR);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_CURSOR):
        if(AT_SEND_WAIT_CURSOR == ntp_send_msg_order)
        {
            ntp_send_at_builder(fsm_ptr, NULL);
            send_transition(AT_SEND_WAIT_SENDOK);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_SENDOK):
        if(AT_SEND_WAIT_SENDOK == ntp_send_msg_order)
        {
            if(BUILDER_FINISHED == esp_ntp_scratchpad.order)
            {
                esp_fsm_state_transition(ST_NTP_RECEIVE);
            }
            else
            {
                esp_fsm_state_transition(ST_NTP_SEND_WAIT);
            }

            send_transition(AT_SEND_FINISHED);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_CLOSED):
        /* Do nothing, If TCP connection gets closed we will receive an error response shortly */
        break;
    case (AT_EV_TCP_SEND_ERROR):
        esp_fsm_state_transition(ST_NTP_CLOSE);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_ntp_send_wait_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TIMEOUT):
        esp_fsm_state_transition(ST_NTP_SEND);
        break;
    case (AT_EV_ERROR):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_ntp_close_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    ntp_close_msg_order = AT_WAITING_FOR_PAYLOAD;
    esp_ntp_scratchpad.order = BUILDER_FINISHED;
}

void esp_ntp_close_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_CLOSED):
        if(AT_WAITING_FOR_PAYLOAD == ntp_close_msg_order)
        {
            ntp_close_msg_order = AT_WAITING_FOR_OK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if(AT_WAITING_FOR_OK == ntp_close_msg_order)
        {
            ntp_close_msg_order = AT_WAITING_FINISHED;
            esp_fsm_state_transition(ST_FSM_IDLE);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_IPD):
        /* TODO: Check this case when more than one +IPD message is received */
        /* Ignore this event */
        break;
    case (AT_EV_ERROR):
        /* TCP connection may have been closed before */
        esp_fsm_state_transition(ST_FSM_IDLE);
        break;
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_ntp_receive_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_IPD):
#if 1
        dprint(("$$$ +IPD: "));
        printHex((uint8_t*)&(esp_ntp_scratchpad.buffer[0]), NTP_PACKET_SIZE);
        dprintln();
#endif
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_state_transition(ST_NTP_CLOSE);
        break;
    }
}



