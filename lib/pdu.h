/* $Id$ */
#ifndef PDU_H
#define PDU_H
#include <time.h>

int str2pdu(char *str, char *pdu, int charset_conv);

int pdu2str(char *pdu, char *str, int charset_conv);

int splitpdu(char *pdu, char *sendr, time_t *date, char *ascii, char *smsc);

void swapchars(char* string); /* Swaps every second character */
    
#endif
