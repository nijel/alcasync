/*
 * alcatool/sms.h
 *
 * sms reading/writing functions
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
#ifndef SMS_H
#define SMS_H
#include <time.h>

#define SMS_UNREAD  0
#define SMS_READ    1
#define SMS_UNSENT  2
#define SMS_SENT    3
#define SMS_ALL     4

typedef struct {
    int pos;
    int stat;
    int len;
    char* raw;
    char* sendr;
    time_t date;
    char* ascii;
    char* smsc;
} SMS;


int delete_sms(int which);

SMS *get_sms(int which);
SMS *get_smss(int state = SMS_ALL);

int send_sms(char *pdu);
int put_sms(char *pdu, int state);

char *get_smsc(void);
void set_smsc(char *smsc);

#endif
