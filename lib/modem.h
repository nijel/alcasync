/*
 * alcatool/modem.h
 *
 * modem initialization, commands and closing
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
#ifndef MODEM_H
#define MODEM_H

#include <termios.h>

#define ERR_MDM_PDU     1
#define ERR_MDM_AT      2
#define ERR_MDM_LOCK    3
#define ERR_MDM_OPEN    4

extern int modem_errno;

//int modem;
extern int rate;
extern int baudrate;

extern char device[100];
extern char lockname[100];
extern char initstring[100];
//char initstring[100]="AT S7=45 S0=0 L1 V1 X4 &c1 E1 Q0";
//char modemname[100];
//char smsc[100];
//char mode[10];
//struct termios oldtio;

/* write command to modem and wait for answer until:
     1. timeout is reached
     2. OK or ERROR is received
     3. expected string is received
   parameters:
     command - what to execute
     answer - buffer for answer
     max - size of answer buffer
     timeout - how long to wait (*0.1 seconds)
     expect - expected string
 */

extern int modem_cmd(char* command,char* answer,int max,int timeout,char* expect);

extern void modem_start_raw(void);

#define modem_stop_raw() modem_setup()

extern int modem_send_raw(unsigned char *buffer,int len);
extern int modem_read_raw(unsigned char *buffer,int len);

extern void modem_setup(void); /* setup serial port */

extern int modem_init(void);

extern int modem_open(void); // Open the serial port

extern void modem_close(void);

#endif
