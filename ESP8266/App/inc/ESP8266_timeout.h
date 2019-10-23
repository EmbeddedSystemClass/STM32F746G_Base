/*
 * ESP8266_timeout.h
 *
 *  Created on: 16/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_TIMEOUT_H_
#define ESP8266_TIMEOUT_H_

#include <Arduino.h>

void esp_set_timeout (uint32_t timeout);

bool esp_timeout_expired (void);

#endif /* ESP8266_TIMEOUT_H_ */
