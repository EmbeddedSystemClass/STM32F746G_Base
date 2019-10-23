/*
 * ESP8266_FSM_config.h
 *
 *  Created on: 22/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_FSM_CONFIG_H_
#define ESP8266_FSM_CONFIG_H_

#define ESP8266_TX_BUFF_SIZE        128     /* ESP8266 Tx Buffer Size */
#define AT_PARSER_BUFF_SIZE         128

#define ESP8266_MAX_SSID_BUFF       WEB_MAX_SSID        /* ESP8266 maximum SSID that will be saved */
#define ESP8266_SSID_LEN            WEB_MAX_SSID_LEN    /* ESP8266 maximum length for each SSID */

#define BACKLOG_DELAY               10      /* Time to wait (sec) until next backlog data is sent */
#define AP_JOIN_DELAY               30      /* Time to wait (sec) until next AP join attempt is executed */
#define RTC_UPDATE_DELAY            WEB_RTC_SYNC_PERIOD /* Time to wait (sec) for next RTC Sync */
#define IP_UPDATE_DELAY             60      /* Time to wait (sec) for next IP Sync */

/* Enable/ Disable State transitions debug mode */
#define ESP8266_FSM_DEBUG           (0)
#define ESP8266_FSM_DBG_TRANS       (1)

/* Enable/ Disable Queue debug mode */
#define AT_EV_QUEUE_DEBUG           (0)
#define EVENT_QUEUE_LEN             (16)

/* Enable/Disable AT Parser debug */
#define ESP8266_AT_PARSER_DBG       (0)
#define ESP8266_AT_PARSER_DBG_UXP   (0)
#define ESP8266_ECHO_CMD_DBG        (1)

#endif /* ESP8266_FSM_CONFIG_H_ */
