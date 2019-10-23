/*
 * ESP8266_Utils.h
 *
 *  Created on: 16/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_UTILS_H_
#define ESP8266_UTILS_H_

#include "ESP8266_FSM_config.h"

#define ARDBUFFER (16)

#include <stdarg.h>
#include <Arduino.h>
#include "../utils.h"
#include "../dprint.h"

int esp_printf(char *fmt, ... );
void esp_printf(const __FlashStringHelper *fmt, ...);
bool get_word_from_line(const char *line, uint32_t lineSize, const char delimiterA, const char delimiterB, char *word);

#endif /* ESP8266_UTILS_H_ */
