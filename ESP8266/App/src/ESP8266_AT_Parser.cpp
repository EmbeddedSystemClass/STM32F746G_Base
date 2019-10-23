/*
 * ESP8266_AT_Parser.cpp
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#include "ESP8266_AT_Parser.h"
#include "ESP8266_FSM_Events.h"
#include "ESP8266_FSM_config.h"
#include "ESP8266_Utils.h"

extern osPoolId  eventPoolId;
extern osMessageQId eventQueueID;

typedef enum
{
    ST_PARSER_LEAD_CR = 0,
    ST_PARSER_LEAD_LF,
    ST_PARSER_PAYLOAD,
    ST_PARSER_TAIL_LF,
} esp_parser_state_t;

typedef struct
{
    uint8_t  buff[AT_PARSER_BUFF_SIZE];
    uint8_t* ntp_buff;
    uint16_t len;
    uint16_t idx;
    esp_parser_state_t state;
} esp_parser_scratchpad_t;

typedef struct
{
    char ssid_matrix[ESP8266_MAX_SSID_BUFF][ESP8266_SSID_LEN];
    uint8_t current_ssid_buffer;
} esp_ap_array;

typedef struct
{
    bool validDate;
    Time date;
} esp_date_t;


/* Private definitions */
static uint8_t esp_parser_run(void);
static uint8_t handle_complete_command(char* data, uint16_t len);
static uint8_t handle_version_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);
static uint8_t handle_tcp_status_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);
static uint8_t handle_ipd_receive_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);
static uint8_t handle_ipv4_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);
static uint8_t handle_mac_address_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);
static uint8_t handle_ntp_receive_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);
static uint8_t handle_access_point(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);
static uint8_t handle_receive_date(esp_event_data_t* ev_data_ptr, char* data, uint16_t len);

/* Main parser scratchpad */
static esp_parser_scratchpad_t esp_parser_scratchpad;
static esp_ap_array esp_ap_list_scratchpad;
static esp_date_t esp_date;

void esp_parser_setup(void)
{
    dprintfln("[ESP] Buff length: %d", sizeof(esp_parser_scratchpad.buff));
    esp_parser_reset();

    /* Point to first element in the array of SSIDs */
    esp_ap_list_scratchpad.current_ssid_buffer = 0;
}

void esp_parser_ntp_buffer(char* dest)
{
    /* Set NTP buffer */
    esp_parser_scratchpad.ntp_buff = (uint8_t*)dest;
}

void esp_parser_loop(void)
{
    while (0 < ESP8266_UART.available())
    {
        uint8_t free_space = sizeof(esp_parser_scratchpad.buff) - esp_parser_scratchpad.len;

        /* If there is enough space on the buffer */
        if (0 < free_space)
        {
            /* Copy data into the buffer */
            uint8_t bytes_read = ESP8266_UART.readBytesUntil('\n', &esp_parser_scratchpad.buff[esp_parser_scratchpad.len], free_space);

            if(0 < bytes_read)
            {
                /* Increase length */
                esp_parser_scratchpad.len += bytes_read;

                /* Add <CR> character to buffer, it is not added by the readBytesUntil function */
                char last_char = esp_parser_scratchpad.buff[esp_parser_scratchpad.len-1];
                if ('\r' == last_char)
                {
                    esp_parser_scratchpad.buff[esp_parser_scratchpad.len] = '\n';
                    esp_parser_scratchpad.len++;
                }

                /* Execute parser */
                esp_parser_run();
            }
        }
        else
        {
            dprintln("[ESP] Buffer overflow: ");
           esp_parser_reset();
        }

    }
}

/* Reset buffer and counters */
void esp_parser_reset(void)
{
    /* Reset parser buffer */
    memset(&esp_parser_scratchpad.buff[0], 0, sizeof(esp_parser_scratchpad.buff));
    esp_parser_scratchpad.len = 0;
    esp_parser_scratchpad.idx = 0;

    /* Reset parser FSM */
    esp_parser_scratchpad.state = ST_PARSER_LEAD_CR;
}

static uint8_t esp_parser_run(void)
{
    /* Return value */
    uint8_t ret = 1;

    uint8_t cur_char;

    /* cmd_start_idx marks the start of the message */
    uint16_t start_cmd_idx;

    for (start_cmd_idx = 0; esp_parser_scratchpad.idx < esp_parser_scratchpad.len; esp_parser_scratchpad.idx++)
    {
        cur_char = *(&esp_parser_scratchpad.buff[esp_parser_scratchpad.idx]);

        if ('>' == cur_char)
        {
            {
                /* Cursor received, restart parser */
        		esp_event_data_t *ev_data = (esp_event_data_t*)osPoolAlloc(eventPoolId);
        		ev_data->ev = AT_EV_TCP_CURSOR;
        		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
        		{
        			osPoolFree(eventPoolId, ev_data);
        		}

                esp_parser_scratchpad.state = ST_PARSER_LEAD_CR;
            }

            continue;
        }

        switch (esp_parser_scratchpad.state)
        {
            case ST_PARSER_LEAD_CR:
            {
                // Parse until the \r character, then look for the
                // \n character, to represent the start of the response.
                // Any other characters are ignored in this state and
                // may be received from the modem if echo is enabled
                // or otherwise.
                if ('\r' == cur_char)
                {
                    esp_parser_scratchpad.state = ST_PARSER_LEAD_LF;
                }
                else if((cur_char >= ' ') && (cur_char <= '~'))
                {
                    esp_parser_scratchpad.state  = ST_PARSER_PAYLOAD;
                    start_cmd_idx = esp_parser_scratchpad.idx;
                }
                break;
            }
            case ST_PARSER_LEAD_LF:
            {
                if ('\n' == cur_char)
                {
                    esp_parser_scratchpad.state  = ST_PARSER_PAYLOAD;
                    start_cmd_idx = esp_parser_scratchpad.idx + 1;
                }
                else if ('\r' != cur_char)
                {
                    // If we are here there was an \r previously
                    // but no subsequent \n.  Therefore this is not
                    // the start of a response for this module to
                    // announce.  If it is a '\r' then stay in the
                    // same state expecting a '\n' next.  Otherwise,
                    // reset and look for the 'r' again.
                    esp_parser_scratchpad.state = ST_PARSER_LEAD_CR;
                }
                else if((cur_char >= ' ') && (cur_char <= '~'))
                {
                    esp_parser_scratchpad.state  = ST_PARSER_PAYLOAD;
                    start_cmd_idx = esp_parser_scratchpad.idx;
                }
                break;
            }
            case ST_PARSER_PAYLOAD:
            {
                // If the \r character is read, consider that to
                // be the end of expecting the response payload
                if ('\r' == cur_char)
                {
                    esp_parser_scratchpad.state = ST_PARSER_TAIL_LF;
                }
                break;
            }
            case ST_PARSER_TAIL_LF:
            {
                if ('\n' == cur_char)
                {
                    uint16_t data_len = esp_parser_scratchpad.idx - start_cmd_idx - 1;

                    if (data_len > 0)
                    {
                        if(data_len > sizeof(esp_parser_scratchpad.buff))
                        {
                            /* ERROR */
                        }

                        esp_parser_scratchpad.state = ST_PARSER_LEAD_CR;

                        // Process the completed command, which handles
                        // transitioning states, depending on the contents
                        // of the complete command.

                        ret = handle_complete_command((char*)&esp_parser_scratchpad.buff[start_cmd_idx], data_len);
                    }
                    else
                    {
                        // If a zero length command is received, this is \r\n\r\n
                        // from the modem.  In this condition, treating the second
                        // set of delimiters as being a start of message instead,
                        // looking for payload after the second \r\n.  Therefore,
                        // changing state to AT_PARSER_EXPECTING_RESPONSE_PAYLOAD
                        // and resetting the index of the potential start of
                        // command.

                        esp_parser_scratchpad.state = ST_PARSER_PAYLOAD;
                        start_cmd_idx = esp_parser_scratchpad.idx + 1;
                    }

                }
                else if ('\r' != cur_char)
                {
                    // If we are here there was an \r previously
                    // but no subsequent \n.  Therefore this is not
                    // the end of a response for this module to
                    // announce.  If it is a '\r' then stay in the
                    // same state expecting a '\n' next.  Otherwise,
                    // consider all this as part of the payload.
                    esp_parser_scratchpad.state = ST_PARSER_PAYLOAD;

                    /* ERROR */
                }

                break;
            }
        }
    }

    if(1 == ret)
    {
        uint16_t bytes_to_keep = esp_parser_scratchpad.len - start_cmd_idx;

        /***************************************************************************
         * There are bytes to keep and we are in the expecting response
         * payload state or expecting tail LF state.
         * Save the mid-parsing AT command content for the next execute call.
         **************************************************************************/
        if ((0 != bytes_to_keep) && ((ST_PARSER_PAYLOAD == esp_parser_scratchpad.state) || (ST_PARSER_TAIL_LF == esp_parser_scratchpad.state)))
        {
            /* Trim buffer */
            memcpy(&esp_parser_scratchpad.buff[0], &esp_parser_scratchpad.buff[start_cmd_idx], bytes_to_keep);

            /* Fill unused buffer space with zeros */
            memset(&esp_parser_scratchpad.buff[bytes_to_keep], 0, sizeof(esp_parser_scratchpad.buff) - esp_parser_scratchpad.len);

            /* Reset the parse index and length to the adjusted buffer, for future UART data. */
            esp_parser_scratchpad.len = bytes_to_keep;
            esp_parser_scratchpad.idx = bytes_to_keep;
        }
        else
        {
            /* Reset the parser */
            esp_parser_reset();
        }
    }

    return ret;
}

static uint8_t handle_complete_command(char* data, uint16_t len)
{
    uint8_t ret = 1;
    esp_event_data_t *ev_data = (esp_event_data_t*)osPoolAlloc(eventPoolId);
    ev_data->ev = AT_EV_INVALID;

#if ESP8266_AT_PARSER_DBG == 1
    dprint(("[ESP-PAR] Rx << "));
    dwrite(data, len);
    dprintln();
#endif

    if((len >= strlen("OK")) && (0 == memcmp(data, "OK", strlen("OK"))))
    {
        ev_data->ev = AT_EV_OK;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("ready")) && (0 == memcmp(data, "ready", strlen("ready"))))
    {
        ev_data->ev = AT_EV_READY;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("busy")) && (0 == memcmp(data, "busy", strlen("busy"))))
    {
        ev_data->ev = AT_EV_BUSY;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("ERROR")) && (0 == memcmp(data, "ERROR", strlen("ERROR"))))
    {
        ev_data->ev = AT_EV_ERROR;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("FAIL")) && (0 == memcmp(data, "FAIL", strlen("FAIL"))))
    {
        ev_data->ev = AT_EV_FAIL;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("CONNECT")) && (0 == memcmp(data, "CONNECT", strlen("CONNECT"))))
    {
        ev_data->ev = AT_EV_TCP_CONNECT;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("ALREADY CONNECTED")) && (0 == memcmp(data, "ALREADY CONNECTED", strlen("ALREADY CONNECTED"))))
    {
        ev_data->ev = AT_EV_TCP_ALREADY_CONNECT;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("CLOSED")) && (0 == memcmp(data, "CLOSED", strlen("CLOSED"))))
    {
        ev_data->ev = AT_EV_TCP_CLOSED;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("SEND OK")) && (0 == memcmp(data, "SEND OK", strlen("SEND OK"))))
    {
        ev_data->ev = AT_EV_TCP_SENDOK;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("SEND FAIL")) && (0 == memcmp(data, "SEND FAIL", strlen("SEND FAIL"))))
    {
        ev_data.ev = AT_EV_TCP_SEND_ERROR;
        ev_data->ev = AT_EV_READY;
		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    else if((len >= strlen("STATUS:")) && (0 == memcmp(data, "STATUS:", strlen("STATUS:"))))
    {
        ret = handle_tcp_status_data(ev_data, data, len);
    }
    else if((len >= strlen("+CIPSTATUS:")) && (0 == memcmp(data, "+CIPSTATUS:", strlen("+CIPSTATUS:"))))
    {
        ret = 1; /* Do nothing, its just the ACK from the +CIPSTATUS command */
    }
    else if((len >= strlen("AT version")) && (0 == memcmp(data, "AT version", strlen("AT version"))))
    {
        ret = handle_version_data(ev_data, data, len);
    }
    else if((len >= strlen("+IPD,48:")) && (0 == memcmp(data, "+IPD,48:", strlen("+IPD,48:"))))
    {
        ret = handle_ntp_receive_data(ev_data, data + 8, len - 8);
    }
    else if((len >= strlen("+IPD")) && (0 == memcmp(data, "+IPD", strlen("+IPD"))))
    {
        ret = handle_ipd_receive_data(ev_data, data, len);
    }
    else if((len >= strlen("+CIFSR:STAIP,")) && (0 == memcmp(data, "+CIFSR:STAIP,", strlen("+CIFSR:STAIP,"))))
    {
        ret = handle_ipv4_data(ev_data, data, len);
    }
    else if((len >= strlen("+CIFSR:STAMAC,")) && (0 == memcmp(data, "+CIFSR:STAMAC,", strlen("+CIFSR:STAMAC,"))))
    {
        ret = handle_mac_address_data(ev_data, data, len);
    }
    else if((len >= strlen("+CWLAP:")) && (0 == memcmp(data, "+CWLAP:", strlen("+CWLAP:"))))
    {
        ret = handle_access_point(ev_data, data, len);
    }
    else if((len >= strlen("Recv ")) && (0 == memcmp(data, "Recv ", strlen("Recv "))))
    {
        ret = 1; /* Do nothing, its just the ACK from the SEND command */
    }
    else if((len >= strlen("Date: ")) && (0 == memcmp(data, "Date: ", strlen("Date: "))))
    {
        ret = handle_receive_date(ev_data, data, len);
    }
    /* Avoid empty space */
    else if(!((1 == len) && (*data == ' ')))
    {
        /* Command not found, unexpected data */
        dprint(("[ESP-PAR] << "));
        dwrite(data, len);
        dprint(" [");
        printHex((uint8_t*)data, len);
        dprint("]");
        dprintln();
    }

    if (AT_EV_INVALID == ev_data->ev)
    {
		osPoolFree(eventPoolId, ev_data);
    }

    return ret;
}

static uint8_t handle_version_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    uint8_t ret = 1;
    ev_data_ptr->ev = AT_EV_VERSION;

    /* TODO: Parse event data */

    if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
	{
		osPoolFree(eventPoolId, ev_data);
	}

	return ret;
}

static uint8_t handle_tcp_status_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    uint8_t ret = 0;
    char* cpStatus = NULL;
    int val = 0;

    if (ev_data_ptr != NULL)
    {
        /* Clear event data field before writing to it */
        memset((void *) ev_data_ptr->data, 0x00, sizeof(ev_data_ptr->data));

        /* Set starting point */
        cpStatus = strchr(data, ':');

        if(NULL != cpStatus)
        {
            cpStatus++;

            if ('2' <= *cpStatus && '5' >= *cpStatus)
            {
                ev_data_ptr->data[0] = (uint8_t)(*cpStatus - '0');
            }
#if ESP8266_AT_PARSER_DBG == 1
            dprint("[ESP] CIP status << ");
            dwrite(data, len);
            dprintln();
            dprintfln("[ESP] CIP status << %d", ev_data_ptr->data[0]);
#endif
        }

        /* Queue event */
        ev_data_ptr->ev = AT_EV_TCP_STATUS;

		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}

        ret = 1;
    }
    return ret;
}

static uint8_t handle_ipd_receive_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    uint8_t ret = 0;
    uint32_t dataFieldSize = sizeof(esp_event_data_t) - sizeof(esp_event_t);
    int16_t httpStatus = 0;
    char tmpBuffer[8];

    if (ev_data_ptr != NULL)
    {
        ev_data_ptr->ev = AT_EV_TCP_IPD;

        dprint(("[ESP-IPD] << "));
        dwrite(data, len);
        dprintln();

        /* Clear event data field before writing to it */
        memset((void *) ev_data_ptr->data, 0x00, dataFieldSize);
        /* Get connection status from response */
        if (get_word_from_line(data, len, ' ', ' ', tmpBuffer))
        {
            httpStatus = (int16_t)atoi(tmpBuffer);
            /* validate httpStatus range */
            if ((0 < httpStatus) && (600 > httpStatus))
            {
                *(int16_t *)&ev_data_ptr->data = httpStatus;
                ret = 1;
            }
        }

		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    return ret;
}

static uint8_t handle_ntp_receive_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    uint8_t ret = 0;
    if (ev_data_ptr != NULL)
    {
        ev_data_ptr->ev = AT_EV_TCP_IPD;

        dprintfln("*** IPD data len: %d", len);

        /* Clear event data field before writing to it */
        memset(esp_parser_scratchpad.ntp_buff, 0x00, 48);

        /* Fill buffer */
        memcpy(esp_parser_scratchpad.ntp_buff, data, 48);

		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}

        ret = 1;
    }
    return ret;
}

static uint8_t handle_access_point(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    uint8_t ret = 0;
    if (ev_data_ptr != NULL)
    {
        ev_data_ptr->ev = AT_EV_WIFI_CWLAP;
        /* Equivalent to NULL address / NULL pointer  */
        *(uint32_t *)(&ev_data_ptr->data[0]) = 0;
        /* Clear event data field before writing to it */
        memset((void *) (&esp_ap_list_scratchpad.ssid_matrix[esp_ap_list_scratchpad.current_ssid_buffer][0]), 0x00, ESP8266_SSID_LEN);
        /* Get SSID name and store in selected buffer */
        if (get_word_from_line(data, len, '"', '"', (&esp_ap_list_scratchpad.ssid_matrix[esp_ap_list_scratchpad.current_ssid_buffer][0])))
        {
            /* Pass as first data, the address where this SSID was stored */
            *(uint32_t *)(&ev_data_ptr->data[0]) = (uint32_t)(&esp_ap_list_scratchpad.ssid_matrix[esp_ap_list_scratchpad.current_ssid_buffer++][0]);
            /* Where are in the last array? if so, reset index buffer */
            if (ESP8266_MAX_SSID_BUFF <= esp_ap_list_scratchpad.current_ssid_buffer)
            {
                esp_ap_list_scratchpad.current_ssid_buffer = 0;
            }
            ret = 1;
        }

		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    return ret;
}

static uint8_t handle_ipv4_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    uint8_t ret = 0;
    uint8_t tempOctet = 0;
    uint8_t idx = 0;
    char* nextDot = NULL;
    char* prevDot = NULL;

#if ESP8266_AT_PARSER_DBG == 1
    dprint("[ESP] IP event << ");
    dwrite(data, len);
    dprintln();
#endif

    int val = 0;

    /* WWW.XXX.YYY.ZZZ\0 */
    char tmpBuffer[16];

    if (ev_data_ptr != NULL)
    {
        /* Fill buffer with zeroes */
        memset(tmpBuffer, 0x00, sizeof(tmpBuffer));

        /* Clear event data field before writing to it */
        ev_data_ptr->ev = AT_EV_LOCAL_IP;
        memset((void *) ev_data_ptr->data, 0x00, sizeof(ev_data_ptr->data));

        /* Get IP value from response */
        if (get_word_from_line(data, len, '"', '"', tmpBuffer))
        {
            /* Set starting point */
            prevDot = &tmpBuffer[0];

#if ESP8266_AT_PARSER_DBG == 1
            dprintfln("tmpBuffer = %s", (char*)&tmpBuffer[0]);
#endif

            /* Parse 4 octets */
            for(idx = 0; idx < 4; idx++)
            {
                /* Get octets */
                nextDot = strchr(prevDot, '.');

#if ESP8266_AT_PARSER_DBG == 1
                dprintfln("prevDot = %d", prevDot);
                dprintfln("nextDot = %d", nextDot);
#endif
                /* First 3 octets */
                if(NULL != nextDot)
                {
                    if(1 == string_to_int(prevDot, &val, (nextDot - prevDot), 1, 0))
                    {
                        prevDot = nextDot + 1;
                        ev_data_ptr->data[idx] = (uint8_t)val;
                    }
                }
                /* Last octet (4th)*/
                else if(3 == idx)
                {
                    if(1 == string_to_int(prevDot, &val, strlen(prevDot), 1, 0))
                    {
                        ev_data_ptr->data[idx] = (uint8_t)val;
                        /* Last octet parsed correctly, parse was successful */
                        ret = 1;
                    }
                }
                /* Error, abort parsing */
                else
                {
                    /* Send data empty */
                    memset((void *) ev_data_ptr->data, 0x00, sizeof(ev_data_ptr->data));
                    break;
                }

#if ESP8266_AT_PARSER_DBG == 1
                dprintfln("Octet #%d = %d", idx, val);
#endif
            }
        }

#if ESP8266_AT_PARSER_DBG == 1
        dprint("[ESP] IP Data << ");
        printHex(&ev_data_ptr->data[0], sizeof(ev_data_ptr->data));
        dprintln();
#endif

		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    return ret;
}

static uint8_t handle_mac_address_data(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    uint8_t ret = 0;
    uint8_t tempOctet = 0;
    uint8_t idx = 0;
    char* nextDot = NULL;
    char* prevDot = NULL;

#if ESP8266_AT_PARSER_DBG == 1
    dprint("[ESP] MAC event << ");
    dwrite(data, len);
    dprintln();
#endif

    int val = 0;

    /* AA:BB:CC:DD:EE:FF \0 */
    char tmpBuffer[18];

    if (ev_data_ptr != NULL)
    {
        /* Fill buffer with zeroes */
        memset(tmpBuffer, 0x00, sizeof(tmpBuffer));

        /* Clear event data field before writing to it */
        ev_data_ptr->ev = AT_EV_LOCAL_MAC;
        memset((void *) ev_data_ptr->data, 0x00, sizeof(ev_data_ptr->data));

        /* Get IP value from response */
        if (get_word_from_line(data, len, '"', '"', tmpBuffer))
        {
            /* Set starting point */
            prevDot = &tmpBuffer[0];

#if ESP8266_AT_PARSER_DBG == 1
            dprintfln("tmpBuffer = %s", (char*)&tmpBuffer[0]);
#endif

            /* Parse 4 octets */
            for(idx = 0; idx < 6; idx++)
            {
                /* Get octets */
                nextDot = strchr(prevDot, ':');

#if ESP8266_AT_PARSER_DBG == 1
                dprintfln("prevDot = %d", prevDot);
                dprintfln("nextDot = %d", nextDot);
#endif
                /* First 3 octets */
                if(NULL != nextDot)
                {
                    if(1 == hex_string_to_int(prevDot, &val, (nextDot - prevDot), 1, 0))
                    {
                        prevDot = nextDot + 1;
                        ev_data_ptr->data[idx] = (uint8_t)val;
                    }
                }
                /* Last octet (4th)*/
                else if(5 == idx)
                {
                    if(1 == hex_string_to_int(prevDot, &val, strlen(prevDot), 1, 0))
                    {
                        ev_data_ptr->data[idx] = (uint8_t)val;
                        /* Last octet parsed correctly, parse was successful */
                        ret = 1;
                    }
                }
                /* Error, abort parsing */
                else
                {
                    /* Send data empty */
                    memset((void *) ev_data_ptr->data, 0x00, sizeof(ev_data_ptr->data));
                    break;
                }

#if ESP8266_AT_PARSER_DBG == 1
                dprintfln("Address byte #%d = %d", idx, val);
#endif
            }
        }

#if ESP8266_AT_PARSER_DBG == 1
        dprint("[ESP] MAC Data << ");
        printHex(&ev_data_ptr->data[0], sizeof(ev_data_ptr->data));
        dprintln();
#endif

		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    return ret;
}


static uint8_t handle_receive_date(esp_event_data_t* ev_data_ptr, char* data, uint16_t len)
{
    String date = String(data);
    Time *timePtr = &esp_date.date;

#if ESP8266_AT_PARSER_DBG == 1
    dprintf("[ESP_PARSER] Date buffer: %s\r\n", data);
#endif

    /* Clear date structure */
    memset((void *)timePtr, 0x00, sizeof(Time));
    esp_date.validDate = true;

    date.toUpperCase();
    // example:
    // date: Thu, 19 Nov 2015 20:25:40 GMT
    /* Parse DOW */
    String dayOfWeek = date.substring(6, 9);
    /* Day of the week */
    if (0 == strcmp("MON", dayOfWeek.c_str()))
    {
        timePtr->dow = 1;
    }
    else if (0 == strcmp("TUE", dayOfWeek.c_str()))
    {
        timePtr->dow = 2;
    }
    else if (0 == strcmp("WED", dayOfWeek.c_str()))
    {
        timePtr->dow = 3;
    }
    else if (0 == strcmp("THU", dayOfWeek.c_str()))
    {
        timePtr->dow = 4;
    }
    else if (0 == strcmp("FRI", dayOfWeek.c_str()))
    {
        timePtr->dow = 5;
    }
    else if (0 == strcmp("SAT", dayOfWeek.c_str()))
    {
        timePtr->dow = 6;
    }
    else if (0 == strcmp("SUN", dayOfWeek.c_str()))
    {
        timePtr->dow = 7;
    }
    else
    {
        esp_date.validDate = false;
        /* Something went wrong!! */
        dprint("Bad day of the week parsing");
    }

    /* Parse day */
    timePtr->date = date.substring(11, 13).toInt();

    /* Validate day */
    if ((((timePtr->date < 1) || (timePtr->date > 31))) && (!esp_date.validDate))
    {
        dprint("Bad date parsing");
    }
    else
    {
        /* Parse Month */
        String month = date.substring(14, 17);
        if (0 == strcmp("JAN", month.c_str()))
        {
            timePtr->mon = 1;
        }
        else if (0 == strcmp("FEB", month.c_str()))
        {
            timePtr->mon = 2;
        }
        else if (0 == strcmp("MAR", month.c_str()))
        {
            timePtr->mon = 3;
        }
        else if (0 == strcmp("APR", month.c_str()))
        {
            timePtr->mon = 4;
        }
        else if (0 == strcmp("MAY", month.c_str()))
        {
            timePtr->mon = 5;
        }
        else if (0 == strcmp("JUN", month.c_str()))
        {
            timePtr->mon = 6;
        }
        else if (0 == strcmp("JUL", month.c_str()))
        {
            timePtr->mon = 7;
        }
        else if (0 == strcmp("AUG", month.c_str()))
        {
            timePtr->mon = 8;
        }
        else if (0 == strcmp("SEP", month.c_str()))
        {
            timePtr->mon = 9;
        }
        else if (0 == strcmp("OCT", month.c_str()))
        {
            timePtr->mon = 10;
        }
        else if (0 == strcmp("NOV", month.c_str()))
        {
            timePtr->mon = 11;
        }
        else if (0 == strcmp("DEC", month.c_str()))
        {
            timePtr->mon = 12;
        }
        else
        {
            esp_date.validDate = false;
            /* Something went wrong!! */
            dprint("Bad month parsing");
        }

        /* Parse Year*/
        timePtr->year = date.substring(18, 22).toInt();

        /* Parse Hour, minutes and seconds (UTC) */
        timePtr->hour = date.substring(23, 25).toInt();
        timePtr->min = date.substring(26, 28).toInt();
        timePtr->sec = date.substring(29, 31).toInt();

        /* Validate values */
        if ((timePtr->hour > 23) || (timePtr->min > 59) || (timePtr->sec > 59))
        {
            dprint("Bad hour, min or sec parsing");
        }
    }
#if (ESP8266_AT_PARSER_DBG == 1)
    dprintf("Day of Week: %s\r\n",  esp_date.date.dow == 1 ? "Mon" :
                                    esp_date.date.dow == 2 ? "Tue" :
                                    esp_date.date.dow == 3 ? "Wed" :
                                    esp_date.date.dow == 4 ? "Thu" :
                                    esp_date.date.dow == 5 ? "Fri" :
                                    esp_date.date.dow == 6 ? "Sat" :
                                    esp_date.date.dow == 7 ? "Sun" : "Inv");
    dprintf("Date: %d\r\n", esp_date.date.date);
    dprintf("Month: %s\r\n",    esp_date.date.mon == 1 ? "Jan" :
                                esp_date.date.mon == 2 ? "Feb" :
                                esp_date.date.mon == 3 ? "Mar" :
                                esp_date.date.mon == 4 ? "Apr" :
                                esp_date.date.mon == 5 ? "May" :
                                esp_date.date.mon == 6 ? "Jun" :
                                esp_date.date.mon == 7 ? "Jul" :
                                esp_date.date.mon == 8 ? "Aug" :
                                esp_date.date.mon == 9 ? "Sep" :
                                esp_date.date.mon == 10 ? "Oct" :
                                esp_date.date.mon == 11 ? "Nov" :
                                esp_date.date.mon == 12 ? "Dec" : "Inv");
    dprintf("Year: %d\r\n", esp_date.date.year);
    dprintf("Hour: %d:%d:%d\r\n", esp_date.date.hour, esp_date.date.min,esp_date.date.sec);
#endif

    if ((esp_date.validDate) && (NULL != ev_data_ptr))
    {
        ev_data_ptr->ev = AT_EV_TIME;
        /* Pass as first data, the address where this time structure was stored */
        *(uint32_t *)(&ev_data_ptr->data[0]) = (uint32_t)(&esp_date.date);

		if (osOK != osMessagePut(eventQueueID, (uint32_t)ev_data_ptr, osWaitForever))
		{
			osPoolFree(eventPoolId, ev_data);
		}
    }
    return esp_date.validDate;
}

