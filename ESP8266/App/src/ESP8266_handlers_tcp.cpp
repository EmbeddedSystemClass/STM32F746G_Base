/*
 * ESP8266_handlers_common.cpp
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#include "ESP8266_handlers_tcp.h"
#include "ESP8266_AT_CMD.h"
#include "ESP8266_Utils.h"
#include "../sensorsHandler.h"
#include "../logHandler.h"

#define TCP_SEND_FSM_DBG (0)

typedef enum
{
    BUILDER_SEND_HTTP_PATH,                   /* +CIPSEND:<len> */
    BUILDER_SEND_HTTP_PATH_DATA,              /* > POST /api/data/ HTTP/1.1 */
    BUILDER_SEND_HEADER_HOST,                 /* +CIPSEND:<len> */
    BUILDER_SEND_HEADER_HOST_DATA,            /* > Host: 192.168.253.31 */
    BUILDER_SEND_HEADER_TOKEN,                /* +CIPSEND:<len> */
    BUILDER_SEND_HEADER_TOKEN_DATA,           /* > X-ARDUINOTOKEN: e5242fe888b7473c871f */
    BUILDER_SEND_HEADER_CONTENT_LENGTH,       /* +CIPSEND:<len> */
    BUILDER_SEND_HEADER_CONTENT_LENGTH_DATA,  /* > Content-Length: 73 */
    BUILDER_SEND_CONTENT,                     /* +CIPSEND:<len> */
    BUILDER_SEND_CONTENT_DATA,                /* > field1=1482369605&field2=26.60&field3=28.00&field4=-127.00&field5=-127.00 */
    BUILDER_FINISHED,
} at_tcp_send_message_order_t;

typedef enum
{
    AT_SEND_WAIT_FOR_OK,
    AT_SEND_WAIT_CURSOR,
    AT_SEND_WAIT_SENDOK,
    AT_SEND_FINISHED,
} at_tcp_send_builder_order_t;


typedef struct
{
        at_tcp_send_message_order_t order;
        char buffer[ESP8266_TCP_SEND_BUFF_LEN];
        uint16_t len;
} tcp_send_scratchpad_t;

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
static at_common_message_order_t tcp_start_msg_order = AT_WAITING_FOR_PAYLOAD;
static at_common_message_order_t tcp_start_status_msg_order = AT_WAITING_FOR_PAYLOAD;
static at_common_message_order_t tcp_close_status_msg_order = AT_WAITING_FOR_PAYLOAD;
static at_tcp_send_builder_order_t tcp_send_msg_order = AT_SEND_WAIT_FOR_OK;
static at_common_message_order_t tcp_close_msg_order = AT_WAITING_FOR_PAYLOAD;

static tcp_send_scratchpad_t send_scratchpad;

static void send_transition(at_tcp_send_builder_order_t state);

static void send_transition(at_tcp_send_builder_order_t state)
{
#if TCP_SEND_FSM_DBG == 1
    dprint("SEND-FSM: [");
    dprint(sendLabels[tcp_send_msg_order]);
    dprint("] -> [");
    dprint(sendLabels[state]);
    dprintln("]");
#endif
    tcp_send_msg_order = state;
}

void tcp_start_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    if (NULL == at_cmd)
    {
#if ESP8266_TCP_HARDCODED == 0
        esp_printf(AT_CMD_TCP_START, fsm_ptr->server, fsm_ptr->port, AT_CMD_EOL);
#else
        esp_printf(AT_CMD_TCP_START, "192.168.253.31", fsm_ptr->port, AT_CMD_EOL);
#endif
    }
}

void tcp_send_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    uint32_t data_len;
    switch(send_scratchpad.order)
    {
    case (BUILDER_SEND_HTTP_PATH):
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "POST %s HTTP/1.1\r\n", fsm_ptr->path);

#if ESP8266_HTTP_DBG == 1
        /* Print request Header */
        dprint(("[HTTP] Header: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HTTP_PATH_DATA;
        break;
    case (BUILDER_SEND_HTTP_PATH_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_SEND_HEADER_HOST;
        break;
    case (BUILDER_SEND_HEADER_HOST):

#if ESP8266_TCP_HARDCODED == 0
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "Host: %s\r\n", fsm_ptr->server);
#else
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "Host: %s\r\n", "192.168.253.31");
#endif

#if ESP8266_HTTP_DBG == 1
        /* Print request Host */
        dprint(("[HTTP] Host: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HEADER_HOST_DATA;
        break;
    case (BUILDER_SEND_HEADER_HOST_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_SEND_HEADER_TOKEN;
        break;
    case (BUILDER_SEND_HEADER_TOKEN):
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "X-ARDUINOTOKEN: %s\r\n", fsm_ptr->token);

#if ESP8266_HTTP_DBG == 1
        /* Print request token */
        dprint(("[HTTP] Token: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HEADER_TOKEN_DATA;
        break;
    case (BUILDER_SEND_HEADER_TOKEN_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_SEND_HEADER_CONTENT_LENGTH;
        break;
    case (BUILDER_SEND_HEADER_CONTENT_LENGTH):
        data_len = strlen(sensorsData_to_urlencodedstring(&fsm_ptr->data));
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "Content-Length: %d\r\n\r\n", data_len);

#if ESP8266_HTTP_DBG == 1
        /* Print request Length */
        dprint(("[HTTP] Length: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HEADER_CONTENT_LENGTH_DATA;
        break;
    case (BUILDER_SEND_HEADER_CONTENT_LENGTH_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_SEND_CONTENT;
        break;
    case (BUILDER_SEND_CONTENT):
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "%s", sensorsData_to_urlencodedstring(&fsm_ptr->data));

#if ESP8266_HTTP_DBG == 1
       /* Print request payload */
        dprint(("[HTTP] Payload: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_CONTENT_DATA;
        break;
    case (BUILDER_SEND_CONTENT_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_FINISHED;
        break;
    default:
        /* Do nothing */
        break;
    }

#if ESP8266_TCP_SEND_DBG == 1
    /* Debug data to send */
    switch(send_scratchpad.order)
    {
    case (BUILDER_SEND_HTTP_PATH_DATA):
        dprintln(("\r\n\r\n### HTTP Request start"));
    case (BUILDER_SEND_HEADER_HOST_DATA):
    case (BUILDER_SEND_HEADER_TOKEN_DATA):
    case (BUILDER_SEND_HEADER_CONTENT_LENGTH_DATA):
    case (BUILDER_SEND_CONTENT_DATA):
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        break;
    case (BUILDER_FINISHED):
        dprintln(("### HTTP Request end\r\n"));
        break;
    default:
        /* Do nothing */
        break;
    }
#endif

}

void tcp_req_time_start(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    if (NULL == at_cmd)
    {
        esp_printf(AT_CMD_TCP_START, TCP_TIME_SERVER, TCP_TIME_PORT, AT_CMD_EOL);
    }
}

void tcp_req_time_send_at_builder(esp_fsm_scratchpad_t* fsm_ptr, const char* at_cmd)
{
    uint32_t data_len;
    switch(send_scratchpad.order)
    {
    case (BUILDER_SEND_HTTP_PATH):
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "GET %s HTTP/1.1\r\n", "/");

#if ESP8266_HTTP_DBG == 1
        /* Print request Header */
        dprint(("[HTTP] Header: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HTTP_PATH_DATA;
        break;
    case (BUILDER_SEND_HTTP_PATH_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_SEND_HEADER_HOST;
        break;
    case (BUILDER_SEND_HEADER_HOST):
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "Host: %s\r\n", TCP_TIME_SERVER);

#if ESP8266_HTTP_DBG == 1
        /* Print request Host */
        dprint(("[HTTP] Host: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HEADER_HOST_DATA;
        break;
    case (BUILDER_SEND_HEADER_HOST_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_SEND_HEADER_TOKEN;
        break;
    case (BUILDER_SEND_HEADER_TOKEN):
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "User-Agent: %s\r\n", "arduino");

#if ESP8266_HTTP_DBG == 1
        /* Print request Length */
        dprint("[HTTP] TOKEN: ");
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HEADER_TOKEN_DATA;
        break;
    case (BUILDER_SEND_HEADER_TOKEN_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_SEND_HEADER_CONTENT_LENGTH;
        break;
    case (BUILDER_SEND_HEADER_CONTENT_LENGTH):
        send_scratchpad.len = sprintf(send_scratchpad.buffer, "Accept: %s\r\n\r\n", "*/*");

#if ESP8266_HTTP_DBG == 1
        /* Print request Length */
        dprint(("[HTTP] Length: "));
        dwrite((char*) send_scratchpad.buffer, send_scratchpad.len);
        dprintln();
#endif

        esp_printf(AT_CMD_TCP_SEND, send_scratchpad.len, AT_CMD_EOL);
        send_scratchpad.order = BUILDER_SEND_HEADER_CONTENT_LENGTH_DATA;
        break;
    case (BUILDER_SEND_HEADER_CONTENT_LENGTH_DATA):
        ESP8266_UART.write((char*) send_scratchpad.buffer, send_scratchpad.len);
        send_scratchpad.order = BUILDER_FINISHED;
        break;

    default:
        /* Do nothing */
        break;
    }
}

void esp_tcp_setup_mux_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        esp_fsm_state_transition(ST_FSM_IDLE);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_tcp_start_status_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    tcp_start_status_msg_order = AT_WAITING_FOR_PAYLOAD;
}

void esp_tcp_start_status_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    static uint8_t status = 0;

    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_STATUS):
        if(AT_WAITING_FOR_PAYLOAD == tcp_start_status_msg_order)
        {
            status = ev_data_ptr->data[0];
            tcp_start_status_msg_order = AT_WAITING_FOR_OK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if(AT_WAITING_FOR_OK == tcp_start_status_msg_order)
        {
            tcp_start_status_msg_order = AT_WAITING_FINISHED;

            /*
             * <stat> status of ESP8266 station interface
             * 2 : ESP8266 station connected to an AP and has obtained IP
             * 3 : ESP8266 station created a TCP or UDP transmission
             * 4 : the TCP or UDP transmission of ESP8266 station disconnected
             * 5 : ESP8266 station did NOT connect to an AP
             */

            if(3 == status)
            {
                esp_fsm_state_transition(ST_TCP_SEND_WAIT);
            }
            else
            {
                esp_fsm_state_transition(ST_FSM_IDLE);
            }
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

void esp_tcp_close_status_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    tcp_close_status_msg_order = AT_WAITING_FOR_PAYLOAD;
}

void esp_tcp_close_status_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    static uint8_t status = 0;

    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_STATUS):
        if(AT_WAITING_FOR_PAYLOAD == tcp_close_status_msg_order)
        {
            status = ev_data_ptr->data[0];
            tcp_close_status_msg_order = AT_WAITING_FOR_OK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if(AT_WAITING_FOR_OK == tcp_close_status_msg_order)
        {
            tcp_close_status_msg_order = AT_WAITING_FINISHED;

            /*
             * <stat> status of ESP8266 station interface
             * 2 : ESP8266 station connected to an AP and has obtained IP
             * 3 : ESP8266 station created a TCP or UDP transmission
             * 4 : the TCP or UDP transmission of ESP8266 station disconnected
             * 5 : ESP8266 station did NOT connect to an AP
             */

            if(4 == status)
            {
                esp_fsm_state_transition(ST_FSM_IDLE);
            }
            else
            {
                esp_fsm_state_transition(ST_TCP_CLOSE);
            }
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

void esp_tcp_start_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    tcp_start_msg_order = AT_WAITING_FOR_PAYLOAD;
    send_scratchpad.order = BUILDER_SEND_HTTP_PATH;

#if 0
    dprintfln("[HTTP] Data to send: %s", sensorsData_to_urlencodedstring(&fsm_ptr->data));
#endif

}

void esp_tcp_start_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_CONNECT):
    case (AT_EV_TCP_ALREADY_CONNECT):
        if(AT_WAITING_FOR_PAYLOAD == tcp_start_msg_order)
        {
            tcp_start_msg_order = AT_WAITING_FOR_OK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if(AT_WAITING_FOR_OK == tcp_start_msg_order)
        {
            tcp_start_msg_order = AT_WAITING_FINISHED;
            esp_fsm_state_transition(ST_TCP_START_STAT);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
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

void esp_tcp_send_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    tcp_send_msg_order = AT_SEND_WAIT_FOR_OK;
}

void esp_tcp_send_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        if(AT_SEND_WAIT_FOR_OK == tcp_send_msg_order)
        {
            send_transition(AT_SEND_WAIT_CURSOR);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_CURSOR):
        if(AT_SEND_WAIT_CURSOR == tcp_send_msg_order)
        {
            tcp_send_at_builder(fsm_ptr, NULL);
            send_transition(AT_SEND_WAIT_SENDOK);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_SENDOK):
        if(AT_SEND_WAIT_SENDOK == tcp_send_msg_order)
        {
            if(BUILDER_FINISHED == send_scratchpad.order)
            {
                esp_fsm_state_transition(ST_TCP_RECEIVE);
            }
            else
            {
                esp_fsm_state_transition(ST_TCP_SEND_WAIT);
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
        esp_fsm_state_transition(ST_TCP_CLOSE_STAT);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_tcp_send_wait_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TIMEOUT):
        esp_fsm_state_transition(ST_TCP_SEND);
        break;
    case (AT_EV_ERROR):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_tcp_close_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    tcp_close_msg_order = AT_WAITING_FOR_PAYLOAD;
    send_scratchpad.order = BUILDER_FINISHED;
}

void esp_tcp_close_handler(esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_CLOSED):
        if(AT_WAITING_FOR_PAYLOAD == tcp_close_msg_order)
        {
            tcp_close_msg_order = AT_WAITING_FOR_OK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if(AT_WAITING_FOR_OK == tcp_close_msg_order)
        {
            tcp_close_msg_order = AT_WAITING_FINISHED;
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

void esp_tcp_receive_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    int16_t httpStatus = -1;
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_IPD):
    {
        httpStatus = *(int16_t *) &ev_data_ptr->data;
        dprintfln("[HTTP] Status: %d", httpStatus);
        if ((201 == httpStatus) || (200 == httpStatus))
        {
            if (markLogAsSent(fsm_ptr->log_idx))
            {
                resetLogFile();
            }

            /* Close connection */
            esp_fsm_state_transition(ST_TCP_CLOSE_STAT);
        }
        else
        {
            esp_fsm_state_transition(ST_FSM_IDLE);
        }
        break;
    }
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_state_transition(ST_TCP_CLOSE_STAT);
        break;
    }
}

void esp_tcp_req_time_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    tcp_start_msg_order = AT_WAITING_FOR_PAYLOAD;
    send_scratchpad.order = BUILDER_SEND_HTTP_PATH;
}

void esp_tcp_req_time_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TCP_CONNECT):
    case (AT_EV_TCP_ALREADY_CONNECT):
        if(AT_WAITING_FOR_PAYLOAD == tcp_start_msg_order)
        {
            tcp_start_msg_order = AT_WAITING_FOR_OK;
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_OK):
        if(AT_WAITING_FOR_OK == tcp_start_msg_order)
        {
            tcp_start_msg_order = AT_WAITING_FINISHED;
            esp_fsm_state_transition(ST_TCP_REQ_TIME_SEND_WAIT);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
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

void esp_tcp_req_time_send_wait_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TIMEOUT):
        esp_fsm_state_transition(ST_TCP_REQ_TIME_SEND);
        break;
    case (AT_EV_ERROR):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_tcp_req_time_send_initializer(esp_fsm_scratchpad_t* fsm_ptr)
{
    tcp_send_msg_order = AT_SEND_WAIT_FOR_OK;
}

void esp_tcp_req_time_send_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_OK):
        if(AT_SEND_WAIT_FOR_OK == tcp_send_msg_order)
        {
            send_transition(AT_SEND_WAIT_CURSOR);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_CURSOR):
        if(AT_SEND_WAIT_CURSOR == tcp_send_msg_order)
        {
            tcp_req_time_send_at_builder(fsm_ptr, NULL);
            send_transition(AT_SEND_WAIT_SENDOK);
        }
        else
        {
            esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        }
        break;
    case (AT_EV_TCP_SENDOK):
        if(AT_SEND_WAIT_SENDOK == tcp_send_msg_order)
        {
            if(BUILDER_FINISHED == send_scratchpad.order)
            {
                esp_fsm_state_transition(ST_TCP_REQ_TIME_RECEIVE);
            }
            else
            {
                esp_fsm_state_transition(ST_TCP_REQ_TIME_SEND_WAIT);
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
        esp_fsm_state_transition(ST_TCP_CLOSE);
        break;
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_error_handler(fsm_ptr, ev_data_ptr);
        break;
    default:
        esp_fsm_unexpected_event(fsm_ptr, ev_data_ptr);
    }
}

void esp_tcp_req_time_recv_handler (esp_fsm_scratchpad_t* fsm_ptr, esp_event_data_t* ev_data_ptr)
{
    Time *timePtr = NULL;
    uint32_t epoch, prev_epoch, difference;
    switch (ev_data_ptr->ev)
    {
    case (AT_EV_TIME):
    {
        timePtr = (Time *)(*(uint32_t *)(&ev_data_ptr->data[0]));
        if (NULL != timePtr)
        {
            prev_epoch = unixEpoch();
            epoch = makeUnix(timePtr);
            if (epoch != 0)
            {
                difference = ((epoch > prev_epoch) ? (epoch - prev_epoch) : (prev_epoch - epoch));
                if (difference > WEB_RTC_MIN_DIFF)
                {
                    if (syncRTC(timePtr))
                    {
                        dprint("RTC updated correctly\r\n");
                    }
                }
                else
                {
                    dprint("No RTC sync needed\r\n");
                }
            }
            else
            {
                dprint("Invalid epoch (Zero value)\r\n");
            }

#if (ESP8266_AT_PARSER_DBG == 1)
            dprintf("Day of Week: %s\r\n",  timePtr->dow == 1 ? "Mon" :
                                            timePtr->dow == 2 ? "Tue" :
                                            timePtr->dow == 3 ? "Wed" :
                                            timePtr->dow == 4 ? "Thu" :
                                            timePtr->dow == 5 ? "Fri" :
                                            timePtr->dow == 6 ? "Sat" :
                                            timePtr->dow == 7 ? "Sun" : "Inv");
            dprintf("Date: %d\r\n", timePtr->date);
            dprintf("Month: %s\r\n",    timePtr->mon == 1 ? "Jan" :
                                        timePtr->mon == 2 ? "Feb" :
                                        timePtr->mon == 3 ? "Mar" :
                                        timePtr->mon == 4 ? "Apr" :
                                        timePtr->mon == 5 ? "May" :
                                        timePtr->mon == 6 ? "Jun" :
                                        timePtr->mon == 7 ? "Jul" :
                                        timePtr->mon == 8 ? "Aug" :
                                        timePtr->mon == 9 ? "Sep" :
                                        timePtr->mon == 10 ? "Oct" :
                                        timePtr->mon == 11 ? "Nov" :
                                        timePtr->mon == 12 ? "Dec" : "Inv");
            dprintf("Year: %d\r\n", timePtr->year);
            dprintf("Hour: %d:%d:%d\r\n", timePtr->hour, timePtr->min,timePtr->sec);
#endif
        }
        esp_fsm_state_transition(ST_FSM_IDLE);
        break;
    }
    case (AT_EV_ERROR):
    case (AT_EV_TIMEOUT):
        esp_fsm_state_transition(ST_TCP_CLOSE);
        break;
    }
}


