/* $Id$ */
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "sms.h"
#include "pdu.h"
#include "modem.h"
#include "common.h"
#include "logging.h"

int delete_sms(int which) {
    char buffer[1024];
    char cmd[100];

    message(MSG_INFO,"Deleting message %d", which);
    sprintf(cmd, "AT+CMGD=%d\r\n", which);
    if (modem_cmd(cmd,buffer,sizeof(buffer)-1,50,0)==0) return 0;
	if (strstr(buffer, "ERROR") != NULL) return 0;
	return 1;
}

SMS *get_smss() {
    char buffer[10000];
    char *data;
    int count = 0;
    SMS *mesg;
    char raw[1024];
    char sendr[1024];
    char ascii[1024];
    char smsc[1024];
    
    message(MSG_INFO,"Reading all messages");
    
    modem_cmd("AT+CMGL\r\n",buffer,sizeof(buffer)-1,50,0);
    
    data = buffer;
    /* how many sms messages are listed? */
    while( (data = strstr(data,"+CMGL:")) != NULL) {
        count++;
        data++; 
    }

    message(MSG_INFO,"Read %d messages", count);
    
    /* allocate array for storing messages */
    mesg = (SMS *)malloc((count + 1) * (sizeof(SMS)));
    chk(mesg);
    mesg[count].pos = -1; /* end symbol */

    /* fill array */    
    data = buffer;
    count = 0;
    while( (data = strstr(data,"+CMGL:")) != NULL) {
        sscanf(data, "+CMGL: %d, %d, , %d\n", &(mesg[count].pos), &(mesg[count].stat), &(mesg[count].len));
        data = strchr(data, '\n');
        sscanf(data,"\n%s\n",raw);
        splitpdu(raw, sendr, &(mesg[count].date), ascii, smsc);
        mesg[count].raw = strdup(raw);
        mesg[count].sendr = strdup(sendr);
        mesg[count].ascii = strdup(ascii);
        mesg[count].smsc = strdup(smsc);
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
    splitpdu(raw, sendr, &((*mesg).date), ascii, smsc);
    (*mesg).raw = strdup(raw);
    (*mesg).sendr = strdup(sendr);
    (*mesg).ascii = strdup(ascii);
    (*mesg).smsc = strdup(smsc);

    return mesg;
}
