/*
 * wifi.h
 *
 *  Created on: Oct 22, 2019
 *      Author: harganda
 */

#ifndef INC_WIFI_H_
#define INC_WIFI_H_

typedef void (*WiFi_TCP_Response_Callback_t) (char* src, uint16_t len);

uint8_t WiFi_Init(void);
uint8_t WiFi_Get_Status(void);
uint8_t WiFi_Get_Version(char* dest, uint16_t len);
uint8_t WiFi_Get_LocalIP(uint32_t* ipaddr);
uint8_t WiFi_JoinAP(char* ssid, char* pass);
uint8_t WiFi_TCP_Connect(char* server, uint16_t port);
uint8_t WiFi_TCP_Send(char* src, uint16_t len);
uint8_t WiFi_TCP_Register_Callback(WiFi_TCP_Response_Callback_t* cb);

#endif /* INC_WIFI_H_ */
