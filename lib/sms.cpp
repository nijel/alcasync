/*
 * alcatool/sms.cpp
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
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "sms.h"
#include "pdu.h"
#include "modem.h"
#include "mobile.h"
#include "common.h"
#include "logging.h"

//TODO:
//at+cmss

int delete_sms(int which) {
    char buffer[1024];
    char cmd[100];

    message(MSG_INFO,"Deleting message %d", which);
    sprintf(cmd, "AT+CMGD=%d\r\n", which);
    if (modem_cmd(cmd,buffer,sizeof(buffer)-1,50,0)==0) return 0;
	if (strstr(buffer, "ERROR") != NULL) return 0;
	return 1;
}

SMS *get_smss(int state = SMS_ALL) {
    char buffer[10000];
    char *data;
    int count = 0;
    SMS *mesg;
    char raw[1024];
    char sendr[1024];
    char ascii[1024];
    char smsc[1024];
    
    message(MSG_INFO,"Reading all messages");
    
    sprintf(raw, "AT+CMGL=%d\r\n", state);
    modem_cmd(raw ,buffer,sizeof(buffer)-1,100,0);
    
    data = buffer;
    /* how many sms messages are listed? */
    while( (data = strstr(data,"+CMGL:")) != NULL) {
        count++;
        data++; 
    }

    message(MSG_INFO,"Read %d messages", count);
    
    /* allocate array for storing messages */
    mesg = (SMS *)malloc((count + 1) * sizeof(SMS));
    chk(mesg);
    mesg[count].pos = -1; /* end symbol */

    /* fill array */    
    data = buffer;
    count = 0;
    while( (data = strstr(data,"+CMGL:")) != NULL) {
        sscanf(data, "+CMGL: %d, %d, , %d\n", &(mesg[count].pos), &(mesg[count].stat), &(mesg[count].len));
        data = strchr(data, '\n');
        sscanf(data,"\n%s\n",raw);
        split_pdu(raw, sendr, &(mesg[count].date), ascii, smsc);
        chk(mesg[count].raw = strdup(raw));
        chk(mesg[count].sendr = strdup(sendr));
        chk(mesg[count].ascii = strdup(ascii));
        message(MSG_DEBUG2, "Message %d (raw): %s \"%s\"", mesg[count].pos, mesg[count].sendr, mesg[count].ascii);
        chk(mesg[count].smsc = strdup(smsc));
        count++;
    }

    return mesg;
}

SMS *get_sms(int which) {
    char buffer[10000];
    char *data;
    SMS *mesg;
    char raw[1024];
    char sendr[1024];
    char ascii[1024];
    char smsc[1024];

    message(MSG_INFO,"Reading message %d", which);
    sprintf(raw, "AT+CMGR=%d\r\n", which);
    if (modem_cmd(raw,buffer,sizeof(buffer)-1,50,0)==0) return NULL;

    /* allocate array for storing message */
    mesg = (SMS *)malloc(sizeof(SMS));
    chk(mesg);


    /* fill array */    
    data = buffer;
    (*mesg).pos = which;
	data = strstr(data, "+CMGR:");
	if (!data) return NULL;
    sscanf(data, "+CMGR: %d, , %d\n", &((*mesg).stat), &((*mesg).len));
    data = strchr(data, '\n');
    sscanf(data,"\n%s\n",raw);
    split_pdu(raw, sendr, &((*mesg).date), ascii, smsc);
    (*mesg).raw = strdup(raw);
    (*mesg).sendr = strdup(sendr);
    (*mesg).ascii = strdup(ascii);
    (*mesg).smsc = strdup(smsc);

    return mesg;
}

int send_sms(char *pdu) {
    char buffer[10000];
    char cmd[1024];
	char *pos;

    message(MSG_INFO,"Sending message");
    sprintf(cmd, "AT+CMGS=%d\r", (strlen(pdu)/2)-1);
    modem_cmd(cmd, buffer, sizeof(buffer)-1, 50, ">");
    sprintf(cmd, "%s\032", pdu);
    modem_cmd(cmd, buffer, sizeof(buffer)-1, 50, "+");
	if (strstr(buffer, "ERROR")) return -1;
	else {
		pos = strstr(buffer, "+CMGS: ");
		pos += 7;
		return atoi(pos);
	}
}

int put_sms(char *pdu, int state) {
    char buffer[10000];
    char cmd[1024];
	char *pos;

    message(MSG_INFO,"Writing message");
    sprintf(cmd, "AT+CMGW=%d,%d\r", (strlen(pdu)/2)-1, state);
    modem_cmd(cmd, buffer, sizeof(buffer)-1, 50, ">");
    sprintf(cmd, "%s\032", pdu);
    modem_cmd(cmd, buffer, sizeof(buffer)-1, 50, NULL);
	if (strstr(buffer, "ERROR")) return -1;
	else {
		pos = strstr(buffer, "+CMGW: ");
		pos += 7;
		return atoi(pos);
	}
}

char *get_smsc(void) {
	char buffer[1024];
    char *s, *t;

	modem_cmd("AT+CSCA?\r\n", buffer, sizeof(buffer) - 1, 50, NULL);
    s = strstr(buffer, "+CSCA: ");
    s += 7;

	if ((t = strchr(s, '\n'))) /* too many parentheses to mke gcc happy */
		*t = '\0';
	
	while (isspace(*s)) s++;
	
	if (!*s || !strcmp(s,"EMPTY"))
		message(MSG_ERROR, "No SMSC set in mobile!");
	
	if (*s++ != '"')
		message(MSG_ERROR, "No left-quote found in SMSC number!");
	
	if (!(t = strrchr(s,'"'))) {
		message(MSG_ERROR, "No right-quote found in SMSC number!");
		if (!(t = strrchr(s,','))) {
			message(MSG_ERROR, "No comma found after SMSC number!");
			t = s + strlen(s) - 1;
		}
	}

	if (s == t)
		message(MSG_ERROR, "No SMSC set in mobile!");
	
	*t = '\0';

    chk(t=strdup(s));
	
    return t;
}

void set_smsc(char *smsc) {
    char command[100];
    char answer[500];
    message(MSG_INFO,"Changing SMSC");
    sprintf(command,"AT+CSCA=\"+%s\"\r\n",smsc);
    modem_cmd(command,answer,sizeof(answer),100,0);
}
