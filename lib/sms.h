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

#define MESSAGE_UNREAD  0
#define MESSAGE_READ    1
#define MESSAGE_UNSENT  2
#define MESSAGE_SENT    3
#define MESSAGE_ALL     4

/** Structure for storing messages
  */
struct MessageData {
    /** position
      */
    int pos;
    /** status
      */
    int stat;
    /** length of raw data
      */
    int len;
    /** raw T-PDU data
      */
    char *raw;
    /** sender/receiver phone
      */
    char *sendr;
    /** date + time of sending
      */
    time_t date;
    /** latin1 text
      */
    char *ascii;
    /** SMSC (Short Message Service Centre)
      */
    char *smsc;
};

/** Deletes message
  */
int delete_sms(int which);

/** Reads mssage
  */
MessageData *get_sms(int which);

/** Reads mssages
  */
MessageData *get_smss(int state = MESSAGE_ALL);

/** Sends message
  */
int send_sms(char *pdu);

/** Store message
  */
int put_sms(char *pdu, int state);

/** Returns SMSC (Short Messages Service Centre)
  */
char *get_smsc(void);

/** Sets SMSC (Short Messages Service Centre)
  */
void set_smsc(char *smsc);

#endif
