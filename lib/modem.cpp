/*
 * alcatool/modem.cpp
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "modem.h"
#include "logging.h"

#define SLEEP_FAIL      100000
#define SLEEP_WAIT      100000
#define SLEEP_INIT      100000

int modem_initialised=0;
int modem_errno;

int modem=0;
int rate;

int baudrate;
char device[100];
char lockname[100];
char initstring[100];

termios oldtio;

int modem_send_raw(unsigned char *buffer,int len) {
    int i=0,fails=0;
    while (i<len){

		if (write(modem,&(buffer[i]),1)!=1) {
			fails++;
		} else {
			fails=0;
			i++;
		}

        if (fails>0) message(MSG_DEBUG2,"Write failed (%d)", fails);

		if (fails>10) {
            message(MSG_ERROR,"Write failed!");
            return 0;
        }
	}
	return len;
}

int modem_read_raw(unsigned char *buffer,int len) {
    return read(modem, buffer, len);
}

int modem_cmd(char* command,char* answer,int max,int timeout,char* expect) {
    int count=0;
    int readcount;
    int toread;
    char tmp[100];
    int counter=0;
    int found=0;

	int len=0,i=0,fails=0;

    answer[0]=0;
    message(MSG_DEBUG,"AT SEND: %s",reform(command,0));
	
	len = strlen(command);
	while (i<len){
//        tcflush(modem, TCIOFLUSH);

		if (write(modem,command+i,1)!=1) {
			fails++;
		} else {
			fails=0;
			i++;
		}

        if (fails>0) message(MSG_DEBUG2,"Write failed (%d)\n", fails);

		if (fails>10) {
            message(MSG_ERROR,"Write failed!\n");
            return 0;
        }
		    
        tcdrain(modem);
	}
	
    /* wait for answer*/
    do {
        counter++;
        toread=max-count-1;
        if (toread>(int)sizeof(tmp)-1)
            toread=(int)sizeof(tmp)-1;
        readcount=read(modem,tmp,toread);
        if (tmp[0]=='\0' || readcount<0) readcount=0;
        tmp[readcount]=0;
        strcat(answer,tmp);
        count+=readcount;
        /* check if it's the expected answer */
        if ((strstr(answer,"OK\r")) ||
                (strstr(answer,"ERROR")))
            found=1;
        if (expect)
            if (expect[0])
                if (strstr(answer,expect))
                    found=1;
		if (found==0) usleep(SLEEP_FAIL);
    }
    while ((found==0) && (counter<timeout));

    /* Read some more in case there is more to read */
    usleep(SLEEP_WAIT);
    toread=max-count-1;
    if (toread>(int)sizeof(tmp)-1)
        toread=sizeof(tmp)-1;
    readcount=read(modem,tmp,toread);
    if (readcount<0)
        readcount=0;
    tmp[readcount]=0;
    strcat(answer,tmp);
    count+=readcount;
    message(MSG_DEBUG,"AT RECV: %s",reform(answer,0));

    return count;
}

void modem_setup(void) {
    termios newtio;
	
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = baudrate|CS8|CREAD|HUPCL|CRTSCTS;
    newtio.c_iflag = IGNBRK;
    newtio.c_oflag = 0;
	newtio.c_lflag = 0;

    memset(newtio.c_cc,0,sizeof(newtio.c_cc));
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;
  
	cfsetispeed(&newtio, baudrate);

    if (cfsetospeed(&newtio, baudrate)|cfsetispeed(&newtio, baudrate))
        message(MSG_ERROR,"Setting of termios baudrate failed!");

    tcflush(modem, TCIOFLUSH);
    tcsetattr(modem,TCSANOW,&newtio);
}

int modem_init(void) {
    char command[100];
    char answer[500];

    tcflush(modem, TCIOFLUSH);

    message(MSG_INFO,"Initializing modem");

    write(modem, "\r", 1);
    write(modem, "A", 1);
    write(modem, "T", 1);
    usleep(SLEEP_INIT);

    while (read(modem, answer, sizeof(command) - 1) > 0)
        ;

    modem_cmd("\r\nAT\033\032\r\n",answer,sizeof(answer),0,NULL);/* ESCAPE, CTRL-Z */
	
    /* perform initialization (if we should do it) */
    if (initstring[0])
    {
        message(MSG_DETAIL,"Sending init: \"%s\"",initstring);
    	sprintf(command,"%s\r\n",initstring);
        modem_cmd(command,answer,sizeof(answer),50,NULL);
    }

    /* check whether there is any modem */
    if (modem_cmd("AT\r\n",answer,sizeof(answer),50,0) == 0) {
        message(MSG_ERROR,"Modem is not reacting to AT command!");
        modem_errno = ERR_MDM_AT;
        return 0;
    }
	modem_initialised = 1;

// alcatel also supports USC2 but it is used only for contacts on sim card and
// return values of some commands (at+csca)
    modem_cmd("AT+CSCS=\"GSM\"\r\n", answer, sizeof(answer), 50, NULL);

// why use this??    
//    message(MSG_DETAIL,"Setting rate");
//    sprintf(command,"AT+IPR=%d\r\n",rate);
//    modem_cmd(command,answer,sizeof(answer),100,0);

    /* select PDU mode for SMSs (Alcatel OT 501 doesn't support any other...) */
    message(MSG_DETAIL,"Selecting PDU mode 0");
    modem_cmd("AT+CMGF=0\r\n",answer,sizeof(answer),100,0);
    if (strstr(answer,"ERROR")) {
        message(MSG_ERROR,"Modem did not accept PDU mode 0");
        modem_errno = ERR_MDM_PDU;
        return 0;
    }
    return 1;
}

int modem_open(void) {
    int pid, i;
    FILE *lock;

    if (modem > 0) { return 1; }

    message(MSG_INFO,"Checking lock for modem %s",lockname);
    lock = fopen(lockname, "r");
    if (lock != NULL) {
        /* lock file exists */
        if (fscanf(lock, "%d", &pid) == 1 && pid > 0) {
            /* pid is written in lock file */
            if (!kill(pid,0)){
                /* process is running */
                message(MSG_ERROR,"Modem is locked by pid %d!",pid);
                modem_errno = ERR_MDM_LOCK;
                return 0;
            }
        }
        fclose(lock);
    }

    i = unlink(lockname);

    if (i && errno != ENOENT)
        message(MSG_WARNING,"Failder lock unlink: %d (%s)\n", errno, strerror(errno));

    if (!(lock = fopen(lockname,"w"))) {
        message(MSG_ERROR,"Can not open lock file for writing!", lockname);
        modem_errno = ERR_MDM_LOCK_OPEN;
        return 0;
    }
    fprintf(lock,"%08d",(int)getpid());
    fclose(lock);
        
    
    message(MSG_INFO,"Opening modem %s",device);

    modem = open(device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
    if (modem <0) {
        perror(device);
        modem_errno = ERR_MDM_OPEN;
        return 0;
    }

    tcgetattr(modem,&oldtio);
	oldtio.c_cflag=(oldtio.c_cflag&~(CBAUD|CBAUDEX))|B0|HUPCL;
	return 1;
}

void modem_close(void) {
    char answer[500];
    /* close modem and remove lockfile */
    if (modem > 0) {
		if (modem_initialised) {
            message(MSG_INFO,"Reseting modem");
            modem_cmd("ATZ\r\n",answer,sizeof(answer),100,NULL);
        }
		tcsetattr(modem,TCSANOW,&oldtio);

        close(modem);
        modem = 0;
        unlink(lockname);
    }
	modem_initialised = 0;
}

