/*
 * ESP8266_AT_Parser.h
 *
 *  Created on: 17/12/2016
 *      Author: Desarrollo 01
 */

#ifndef ESP8266_AT_PARSER_H_
#define ESP8266_AT_PARSER_H_

/* This function should be executed only once on start-up */
void esp_parser_setup(void);

/* This function should be executed periodically */
void esp_parser_loop(void);

/* reser AT Parser buffer */
void esp_parser_reset(void);

/* This function should be executed only once on start-up */
void esp_parser_ntp_buffer(char* dest);

#endif /* ESP8266_AT_PARSER_H_ */
