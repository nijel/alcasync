/* $Id$ */
#ifndef PDU_H
#define PDU_H
#include <time.h>

/* message classes (fot make_pdu) */
#define PDU_CLASS_FLASH     0
#define PDU_CLASS_MOBILE    1
#define PDU_CLASS_SIM       2
#define PDU_CLASS_TERMINATE 3

#define PDU_MAXNUMLEN 20 /* maximum number of digits in GSM number */
#define PDU_MAXBODYLEN 160 /* maximum length of characters in SMS body */

int str2pdu(char *str, char *pdu, int charset_conv);

int pdu2str(char *pdu, char *str, int charset_conv);

int split_pdu(char *pdu, char *sendr, time_t *date, char *ascii, char *smsc);

void make_pdu(char* number, char* message, int deliv_report, int pdu_class, char* pdu);
void make_pdu_smsc(char *smsc, char* number, char* message, int deliv_report, int pdu_class, char* pdu);

void swapchars(char* string); /* Swaps every second character */
    
#endif
