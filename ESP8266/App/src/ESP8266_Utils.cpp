/*
 * ESP8266.cpp
 *
 *  Created on: 16/12/2016
 *      Author: Desarrollo 01
 */

#include "ESP8266_Utils.h"
#include "ESP8266_FSM_config.h"
#include <Arduino.h>

int esp_printf(char *fmt, ... )
{
        char buf[ESP8266_TX_BUFF_SIZE];
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, ESP8266_TX_BUFF_SIZE, fmt, args);
        va_end (args);
        ESP8266_UART.print(buf);

#if ESP8266_ECHO_CMD_DBG == 1
        dprint("CMD: ");
        dprint(buf);
        dprintln();
#endif
}

/* This should work with F() macros */
#if 0
void esp_printf(const __FlashStringHelper *fmt, ...)
{
    char buf[ESP8266_TX_BUFF_SIZE]; // resulting string limited to 128 chars
    va_list args;
    va_start(args, fmt);
#ifdef __AVR__
    vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
    vsnprintf(buf, sizeof(buf), (const char *) fmt, args); // for the rest of the world
#endif
    va_end(args);

#if ESP8266_ECHO_CMD_DBG == 1
        dprint("CMD: ");
        dprint(buf);
        dprintln();
#endif
}
#endif

/* TODO: This function does not consider the destination length */
bool get_word_from_line(const char *line, uint32_t lineSize, const char delimiterA, const char delimiterB, char *word)
{
    bool ret = false;
    char *wordStart = NULL;
    char *wordEnd = NULL;

    /* Validate pointers */
    if ((line != NULL) && (word != NULL))
    {
        /* Search for first delimiter  */
        wordStart = (char *) memchr(line, delimiterA, lineSize);
        /* Found the first delimiter? */
        if (NULL != wordStart)
        {
            /* Point to the first char after the delimiter */
            wordStart++;
            /* Search for second delimiter */
            wordEnd = (char *) memchr(wordStart, delimiterB, lineSize - (wordStart - line));
            /* Found the second delimiter? */
            if (NULL != wordEnd)
            {
                /* More than one character between delimiters? */
                if ((wordEnd - wordStart) > 1)
                {
                    /* NULL-terminated string */
                    *wordEnd = '\0';
                    /* It is not validated word's size, so, please, be sure that word is large enough */
                    strcpy(word, wordStart);
                    ret = true;
                }
            }
        }
    }
    return ret;
}
#undef ARDBUFFER
