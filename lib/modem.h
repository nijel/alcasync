/***************************************************************************
                          modem.h  -  description
                             -------------------
    begin                : Thu Jan 24 2002
    copyright            : (C) 2002 by Michal Cihar
    email                : cihar@email.cz
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef MODEM_H
#define MODEM_H

#include <termios.h>

int modem;
int rate;
int baudrate;
char device[100];
char lockname[100];
char initstring[100];
//char initstring[100]="AT S7=45 S0=0 L1 V1 X4 &c1 E1 Q0";
//char modemname[100];
//char smsc[100];
char mode[10];
struct termios oldtio;

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

int modem_cmd(char* command,char* answer,int max,int timeout,char* expect);

void modem_start_raw();

#define modem_stop_raw() modem_setup()

int modem_send_raw(char *buffer,int len);
int modem_read_raw(char *buffer,int len);

void modem_setup(); /* setup serial port */

void modem_init();

void modem_open(); // Open the serial port

void modem_close();

#endif
