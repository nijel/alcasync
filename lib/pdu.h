/*
 * alcatool/pdu.h
 *
 * PDU decoding/encoding
 *
 * Copyright (c) 2002 by Michal Cihar <cihar@email.cz>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * In addition to GNU GPL this code may be used also in non GPL programs but
 * if and only if programmer/distributor of that code recieves written
 * permission from author of this code.
 *
 */
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
