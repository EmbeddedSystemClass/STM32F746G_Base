/*
 * ESP8266_timeout.cpp
 *
 *  Created on: 16/12/2016
 *      Author: Desarrollo 01
 */

#include "ESP8266_timeout.h"
#include <Arduino.h>

typedef struct {
        uint32_t desiredTimeout;
}esp_timeout_handler;

static esp_timeout_handler esp_timeout_scratchpad;

void esp_set_timeout (uint32_t timeout)
{
    /* Get current time and add desired timeout (in milliseconds) */
    esp_timeout_scratchpad.desiredTimeout = millis() + timeout;
}


bool esp_timeout_expired (void)
{
    return (millis() > esp_timeout_scratchpad.desiredTimeout);
}
