/*
 * settty/main.c
 *
 * set for setting tty
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

#define baudrate B9600

int modem;

char device[]="/dev/ttyS1";
char lockname[]="/var/lock/LCK..ttyS1";

void modem_setup(void) {
    struct termios newtio;
	
    bzero(&newtio, sizeof(newtio));
//    newtio.c_cflag = baudrate|CS8|CREAD|HUPCL|CRTSCTS;
    newtio.c_cflag = baudrate|CS7|PARENB|PARODD|CREAD|HUPCL|CRTSCTS;
    newtio.c_iflag = IGNBRK;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;

    memset(newtio.c_cc,0,sizeof(newtio.c_cc));
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;
  
    cfsetispeed(&newtio, baudrate);

    if (cfsetospeed(&newtio, baudrate)|cfsetispeed(&newtio, baudrate))
        printf("Setting of termios baudrate failed!\n");

    tcflush(modem, TCIOFLUSH);
    tcsetattr(modem,TCSANOW,&newtio);
}

int modem_open(void) {
    int pid, i;
    FILE *lock;

    printf("Checking lock for modem %s\n",lockname);
    lock = fopen(lockname, "r");
    if (lock != NULL) {
        /* lock file exists */
        if (fscanf(lock, "%d", &pid) == 1 && pid > 0) {
            /* pid is written in lock file */
            if (!kill(pid,0)){
                /* process is running */
                printf("Modem is locked by pid %d!\n",pid);
                exit(1);
            }
        }
        fclose(lock);
    }

    i = unlink(lockname);

    if (i && errno != ENOENT)
        printf("Failder lock unlink: %d (%s)\n", errno, strerror(errno));

    if (!(lock = fopen(lockname,"w"))) {
        printf("Can not open lock file (%s) for writing!\n", lockname);
        exit(1);
    }
    fprintf(lock,"%08d",(int)getpid());
    fclose(lock);
        
    
    printf("Opening modem %s\n",device);

    modem = open(device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
    if (modem <0) {
        perror(device);
        exit(1);
    }

    return 1;
}

void modem_close(void) {
    /* close modem and remove lockfile */
    if (modem > 0) {
        close(modem);
        modem = 0;
        unlink(lockname);
    }
}

int main() {
  modem_open();
  modem_setup();
  modem_close();
  return 0;
}
