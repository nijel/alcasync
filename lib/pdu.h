/*
 * alcasync/pdu.h
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
 * if and only if programmer/distributor of that code receives written
 * permission from author of this code.
 *
 */
/* $Id$ */
#ifndef PDU_H
#define PDU_H
#include <time.h>

#define NUM_NAT 0x81 /* phone number type for non-'+' (national) numbers */
#define NUM_INT 0x91 /* phone number type for numbers prefixed by '+' (international) */

#define NUM_TYPE_INT 0x9
#define NUM_TYPE_NAT 0xA
#define NUM_TYPE_CHR 0xD

/*
number types:

1 0 0 0		Unknown
1 0 0 1		International number
1 0 1 0		National number
1 0 1 1		Network specific number
1 1 0 0		Subscriber number
1 1 0 1		Alphanumeric, (coded according to GSM TS 03.38 7?bit default alphabet)
1 1 1 0		Abbreviated number
1 1 1 1		Reserved for extension
*/


/* message classes (fot make_pdu) */
#define PDU_CLASS_FLASH     0
#define PDU_CLASS_MOBILE    1
#define PDU_CLASS_SIM       2
#define PDU_CLASS_TERMINATE 3

#define PDU_MAXNUMLEN 20 /* maximum number of digits in GSM number */
#define PDU_MAXBODYLEN 160 /* maximum length of characters in SMS body */

/** Converts string to PDU data
 */
int str2pdu(const char *str, char *pdu, int charset_conv);

/** Converts PDU data to string
 */
int pdu2str(const char *pdu, char *str, int charset_conv);

/** Splits T-PDU data to message parts
 */
int split_pdu(const char *pdu, char *sendr, time_t *date, char *ascii, char *smsc);

/** Makes T-PDU data
 */
void make_pdu(const char *number, const char *message, int deliv_report, int pdu_class, char *pdu);

/** Makes T-PDU data with included SMSC number
 */
void make_pdu_smsc(const char *smsc, const char *number, const char *message, int deliv_report, int pdu_class, char *pdu);

/** Swaps every second character
 */
void swapchars(const char *string);
    
#endif
