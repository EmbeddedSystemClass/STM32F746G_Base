/*
 * ESP8266_AT_CMD.h
 *
 *  Created on: 22/08/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_AT_CMD_H_
#define ESP8266_AT_CMD_H_

/* All commands have the %s escape sequence character to add the EOL characters <CR><LF> */

/* Basic commands */
#define AT_CMD_EOL                      "\r\n"                              /* EOL <CR><LF> */
#define AT_CMD_TEST                     "AT%s"                              /* Test ESP8266 response */
#define AT_CMD_RST                      "AT+RST%s"                          /* Soft reset */
#define AT_CMD_DISABLE_ECHO             "ATE0%s"                            /* Disable command echo */
#define AT_CMD_VERSION                  "AT+GMR%s"                          /* Print firmware version */
#define AT_CMD_GET_IP_MAC               "AT+CIFSR%s"                        /* Print local IP */

/* WiFi AT Commands */
#define AT_CMD_WIFI_AUTOCONN            "AT+CWAUTOCONN=0%s"                 /* Station mode */
#define AT_CMD_WIFI_MODE                "AT+CWMODE_CUR=1%s"                 /* Station mode */
#define AT_CMD_WIFI_AP_JOIN             "AT+CWJAP_CUR=\"%s\",\"%s\"%s"      /* Join WiFi Access Point */
#define AT_CMD_WIFI_AP_LIST_CFG         "AT+CWLAPOPT=1,3%s"                 /* Configure to print only SSID and ECN on WiFi Access Points list */
#define AT_CMD_WIFI_AP_LIST             "AT+CWLAP%s"                        /* Get AP list */
#define AT_CMD_WIFI_AP_QUERY_SSID       "AT+CWLAP=\"%s\"%s"                 /* Check if SSID is available */
#define AT_CMD_WIFI_AP_QUIT             "AT+CWQAP%s"                        /* Quit access point */
#define AT_CMD_WIFI_DHCP                "AT+CWDHCP_CUR=1,1%s"               /* Enable DHCP for station mode */

/* CIP Commands */
#define AT_CMD_CIP_MUX                  "AT+CIPMUX=0%s"                     /* Single connection */
#define AT_CMD_CIP_STATUS               "AT+CIPSTATUS%s"                    /* Check TCP Connection status */

/* TCP Commands */
#define AT_CMD_TCP_START                "AT+CIPSTART=\"TCP\",\"%s\",%d%s"   /* Start TCP Connection */
#define AT_CMD_TCP_SEND                 "AT+CIPSEND=%d%s"                   /* Send data trough TCP */
#define AT_CMD_TCP_CLOSE                "AT+CIPCLOSE%s"                     /* Close TCP Connection */

/* UDP Commands */
#define AT_CMD_UDP_START                "AT+CIPSTART=\"UDP\",\"%s\",%d,%d%s"   /* Start UDP Connection */
#define AT_CMD_UDP_SEND                 "AT+CIPSEND=%d%s"                         /* Send data trough UDP */
#define AT_CMD_UDP_CLOSE                "AT+CIPCLOSE%s"                           /* Close UDP Connection */


#endif /* ESP8266_AT_CMD_H_ */
