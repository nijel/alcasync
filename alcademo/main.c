/***************************************************************************
                          alctest.c  -  description
                             -------------------
    begin                : Thu Jan 24 20:23:34 CET 2002
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
/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <libgen.h>
#include <string.h>

#include "modem.h"
#include "mobile_info.h"
#include "logging.h"
#include "version.h"
#include "charset.h"
#include "sms.h"
#include "pdu.h"
#include "alcatel.h"

char *progname;

char default_device[] = "/dev/ttyS1";
char default_lock[] = "/var/lock/LCK..%s";
char default_init[] = "AT S7=45 S0=0 L1 V1 X4 &c1 E1 Q0";
int default_rate = 19200;

int action_any = 0;
int action_info = 0;
int action_monitor = 0;
int action_sms = 0;
int action_binary = 0;
int action_read[1000];
int action_read_pos = 0;
int action_del[1000];
int action_del_pos = 0;
int action_list = 0;
int action_implicit_list = 0;
int action_contact = 0;

int action_test = 0;

int binary_mode_active = 0;

void help() {
    printf("This is " NAME " version " VERSION " Copyright (c) " COPYRIGHT "\n");
    printf("Usage:  %s [options]\n", basename(progname));
    printf("Options:\n");
    printf("    -d(dev)/--device=(dev) ... modem device [%s]\n",default_device);
    printf("    -K(fn)/--lock=(fn) ... lock filename [/var/lock/LCK..device]\n");
    printf("    -I(s)/-init=(s) ... init string for modem [%s]\n",default_init);
    printf("    -B(n)/--rate=(n) ... baud rate [%d]\n",default_rate);
    printf("    -q/--quiet ... be more quiet\n");
    printf("    -v/--verbose ... be more verbose\n");
//    printf("    - ...  [%s]\n",default_);
    printf("Actions (at least one MUST be specified):\n");
    printf("    -i/--info ... show mobile info\n");
    printf("    -s[(n)]/--sms[=(n)] ... list SMSs / show selected SMS\n");
//    printf("    -c/--contacts ... list contacts\n");
    printf("    -S/--monitor ... monitor signal strength\n");
//    printf("    - \n");
    printf("    -V/--version ... show version information\n");
    printf("    -h/--help ... this help\n");
    printf("Used symbols:\n");
    printf(" (n) = number\n");
    printf(" (dev) = device name\n");
    printf(" (fn) = file name\n");
    printf(" (s) = string\n");
    printf("\n");
    printf("Return values:\n");
    printf(" 0 - success\n");
    printf(" 1 - modem locked\n");
    printf(" 2 - can not open modem\n");
    printf(" 3 - modem not reacting\n");
    printf(" 4 - modem did not accept PDU mode 0\n");
    printf(" \n");
//    printf(" 10 - message not found\n");
    printf(" \n");
    printf(" 100 - failed allocating memory\n");
    printf(" 200 - no action\n");
    printf(" 201 - bad baud rate\n");
    printf(" 202 - bad device name\n");
    printf(" 255 - displayed help\n");

    exit(255);
}

void version() {
    printf("This is " NAME " version " VERSION " Copyright (c) " COPYRIGHT "\n\n");
    printf("This program is free software; you can redistribute it and/or modify\n");
    printf("it under the terms of the GNU General Public License as published by\n");
    printf("the Free Software Foundation; either version 2 of the License, or\n");
    printf("(at your option) any later version.\n");
    exit(255);
}


void parse_params(int argc, char *argv[]) {
    int result;
    char *devname;

static const struct option longopts[]={
{"info"        ,0,0,'i'},
{"monitor"     ,0,0,'S'},
{"sms"         ,2,0,'s'},

{"read"        ,1,0,'r'},
{"delete"      ,1,0,'x'},
{"list"        ,0,0,'l'},

{"rate"        ,1,0,'B'},
{"init"        ,1,0,'I'},
{"lock"        ,1,0,'K'},
{"device"      ,1,0,'d'},
    
{"quiet"       ,0,0,'q'},
{"verbose"     ,0,0,'v'},

{"help"        ,0,0,'h'},
{"version"     ,0,0,'V'},
{NULL          ,0,0,0  }};


    progname = argv[0];
    /* set some dafaults */    
    strcpy(device,default_device);
    devname = strrchr(device, '/');
    devname++;
    sprintf(lockname, default_lock, devname);
    strcpy(initstring,default_init);
    rate=default_rate;

    while ((result=getopt_long(argc,argv,"bhd:K:I:B:is::cSvqVr:x:lT",longopts,NULL))>0) {
        switch (result) {
            case 'V': 
                version(); 
                break;;
            case 'h': 
                help(); 
                break;;
            case 'd':
                strcpy(device,optarg);
                devname = strrchr(device, '/');
                if (devname == NULL) {
                    message(MSG_ERROR,"Bad device name!");
                    exit(202);
                }
                devname++;
                sprintf(lockname, default_lock, devname);
                break;
            case 'K':
                strcpy(lockname,optarg);
                break;
            case 'I':
                strcpy(initstring,optarg);
                break;
            case 'B':
                rate = atoi(optarg);
                break;
            case 'T':
                action_any = 1;
                action_test = 1;
                break;
            case 'i':
                action_any = 1;
                action_info = 1;
                break;
            case 'l':
                action_list = 1;
                action_any = 1;
                break;
            case 's':
                action_any = 1;
                action_sms = 1;
                if (optarg == NULL) {
                    action_implicit_list = 1;
                    break;
                }
            case 'r':
                action_read[action_read_pos] = atoi(optarg);
                action_read_pos++;
                break;
            case 'x':
                action_del[action_del_pos] = atoi(optarg);
                action_del_pos++;
                break;
            case 'b':
                action_any = 1;
                action_binary = 1;
                break;
            case 'c':
                action_any = 1;
                action_contact = 1;
                break;
            case 'S':
                action_monitor = 1;
                action_any = 1;
                break;
            case 'q':
                if (msg_level<MSG_NONE) msg_level++;
                break;
            case 'v':
                if (msg_level>MSG_ALL) msg_level--;
                break;
        }
        
    }
    
    switch (rate) {
        case 2400:   baudrate=B2400; break;
        case 4800:   baudrate=B4800; break;
        case 9600:   baudrate=B9600; break;
        case 19200:  baudrate=B19200; break;
        case 38400:  baudrate=B38400; break;        
        default: 
            message(MSG_ERROR,"Ivalid baud rate (%d)!", rate);
            exit(201);
    }

    if (action_any == 0) {
        message(MSG_ERROR,"No action wanted!");
        message(MSG_INFO,"Try -h for help");
        exit(200);
    }
}

void shutdown() {
    if (binary_mode_active) {
        alcatel_detach();
        alcatel_done();
    }
    modem_close();
}

void print_hex(char *buffer) {
	static char conv[] = "0123456789ABCDEF";
	int i;
	for (i=1; i<=buffer[0]; i++)
		printf ("%c%c (%c) ", conv[((char)buffer[i] & 0xff) >> 4], conv[buffer[i] & 0x0f], isprint(buffer[i])?buffer[i]:'*');
}

int main(int argc, char *argv[]) {
    char data[1024];
    SMS *sms, *sms1;
    int i, j;

	int *ids, *items, *list;
	alc_type *result;
    alc_type c;
	FIELD *field;

    msg_level = MSG_INFO;

    atexit(shutdown);
    
    parse_params(argc,argv);

    message(MSG_INFO,"This is " NAME " version " VERSION " Copyright (c) " COPYRIGHT);

    modem_open();
    modem_setup(); 
    modem_init();

    if (action_info) {
        get_manufacturer(data,sizeof(data));
        printf ("Manufacturer: %s\n",data);
        
        get_model(data,sizeof(data));
        printf ("Model: %s\n",data);
        
        get_revision(data,sizeof(data));
        printf ("Revision: %s\n",data);
        
        get_sn(data,sizeof(data));
        printf ("Serial no: %s\n",data);

        get_battery(&i,&j);
        printf ("Batery status: %d%%, state: %d\n", j, i);
        
        get_signal(&i,&j);
        printf ("Signal strenght: %s (%d), error rate: %d\n",i == 99 ? "unknown" : mobil_signal_info[i], i, j);
    }

    if (action_sms) {
        if (action_del_pos > 0) {
            for (i = 0; i < action_del_pos; i++){
                if (delete_sms(action_del[i])) {
                    message(MSG_INFO,"Message %d deleted!", action_del[i]);
                } else {
                    message(MSG_ERROR,"Message %d not found!", action_del[i]);
                }
            }
            action_implicit_list = 0;
        }
        if (action_read_pos > 0) {
            for (i = 0; i < action_read_pos; i++){
                sms1 = get_sms(action_read[i]);
                if (sms1) {
                    printf("From: %s\nDate: %sSMSC: %s\nStatus: %d\nPosition: %d\n\n%s\n",
                            (*sms1).sendr,
                            ctime(&((*sms1).date)),
                            (*sms1).smsc,
                            (*sms1).stat,
                            (*sms1).pos,
                            (*sms1).ascii);
                } else {
                    message(MSG_ERROR,"Message %d not found!", action_read[i]);
                }
                if (i != action_read_pos - 1) printf ("-------------------------------------------------------------------------------\n");
            }
            action_implicit_list = 0;
        }
        if (action_list || action_implicit_list) {
            sms = get_smss();
            i = 0;
            while (sms[i].pos != -1) {
                printf("From: %s\nDate: %sSMSC: %s\nStatus: %d\nPosition: %d\n\n%s\n",
                        sms[i].sendr,
                        ctime(&(sms[i].date)),
                        sms[i].smsc,
                        sms[i].stat,
                        sms[i].pos,
                        sms[i].ascii);
                i++;
                if (sms[i].pos != -1) printf ("-------------------------------------------------------------------------------\n");
            }
        }
    }
    
    if (action_monitor) {
        while (1) {
            get_signal(&i,&j);
            printf ("Signal strenght: %s (%d), error rate: %d\n",i == 99 ? "unknown" : mobil_signal_info[i],i,j);
            usleep(100000);
        }
    }

    if (action_binary) {
        alcatel_init();
        binary_mode_active = 1;
        alcatel_attach();
		
		sync_start_session();
#define SYNC_TYPE   ALC_SYNC_TYPE_CONTACTS
#define SYNC        ALC_SYNC_CONTACTS
//        for (c=0; c<=255; c++) {
            sync_select_type(0x08); // 04 08 0c
            c = atoi(argv[2]);
            fprintf(stderr, "08:%02x\n", c);
		    sync_begin_read(c);
            sync_close_session(0x08);
//        }
/*
        sync_select_type(SYNC_TYPE);
		sync_begin_read(SYNC);
		ids = sync_get_ids(SYNC_TYPE);

		message(MSG_INFO, "Received %d ids", ids[0]);
        
		for (i = 1; i <= ids[0]; i++)
            printf ("%02d: %02d\n", i, ids[i]);

--        
        list = sync_get_obj_list(SYNC_TYPE, ALC_LIST_CONTACTS_CAT);
		
        message(MSG_INFO, "Received %d categories:", list[0]);

		for (i = 1; i <= list[0]; i++) {
            result = sync_get_obj_list_item(SYNC_TYPE, ALC_LIST_CONTACTS_CAT, i);
            printf ("%02d: %s\n", list[i],  result);
            free(result);
        }
        
        free(list);

		for (i = 1; i <= ids[0]; i++) {
			message(MSG_DEBUG, "Reading id[%d] = %d", i-1, ids[i]);
			items = sync_get_fields(SYNC_TYPE, ids[i]);
			message(MSG_INFO, "Receiving data for item %d (%d fields)", ids[i], items[0]);
			printf ("Item %d (fields: %d):\n", ids[i], items[0]);
			for (j = 1; j <= items[0]; j++) {
				message(MSG_DEBUG, "items[%d] = %d", j-1, items[j]);
				result = sync_get_field_value(SYNC_TYPE, ids[i], items[j]);
            	message(MSG_DEBUG,"1-Received field info: %s", hexdump(result + 1, result[0], 1));
            	message(MSG_DEBUG,"2-Received field info: %s", reform(result + 1, 1));
				field = decode_field_value(result);
            	printf(" %02d: ",items[j]);
				if (field == NULL) {
					printf ("UNKNOWN TYPE (%02X %02X)\n", result[1], result[2]);
                } else {
                    switch (field->type) {
                        case _date:
                            printf ("%d. %02d. %4d\n", ((DATE *)(field->data))->day, 
                                    ((DATE *)(field->data))->month, 
                                    ((DATE *)(field->data))->year);
                            break;
                        case _time:
                            printf ("%d:%02d:%02d\n", ((TIME *)(field->data))->hour, 
                                    ((TIME *)(field->data))->minute, 
                                    ((TIME *)(field->data))->second);
                            break;
                        case _string:
                            printf("%s\n", (char *)(field->data));
                            break;
                        case _phone:
                            printf("%s\n", (char *)(field->data));
                            break;
                        case _enum:
                            printf("%d\n", *(int *)(field->data));
                            break;
                        case _bool:
                            printf("%s\n", *(int *)(field->data) ? "yes" : "no");
                            break;
                        case _int:
                            printf("%d\n", *(int *)(field->data));
                            break;
                        case _byte:
                            printf("%d\n", *(int *)(field->data));
                            break;
                    }
                }
				free(result);
			}
			free(items);
		}
		free(ids);
		
        sync_close_session(SYNC_TYPE);*/
        alcatel_detach();
        alcatel_done();
        binary_mode_active = 0;
		
    }

    if (action_test) {
    }

    return 0;
}
