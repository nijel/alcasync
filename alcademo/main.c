/*****************************************************************************
 * alctest/main.c - testing program for alcatel communication library        *
 *                                                                           *
 * Copyright (c) 2002 Michal Cihar <cihar at email dot cz>                   *
 *                                                                           *
 * This is  EXPERIMANTAL  implementation  of comunication  protocol  used by *
 * Alcatel  501 (probably also any 50x and 70x) mobile phone. This  code may *
 * destroy your phone, so use it carefully. However whith my phone work this *
 * code correctly. This code assumes following conditions:                   *
 *  - no packet is lost                                                      *
 *  - 0x0F ack doesn't mean anything important                               *
 *  - data will be recieved as they are expected                             *
 *  - no error will appear in transmission                                   *
 *  - all magic numbers mean that, what I thing that they mean ;-)           *
 *                                                                           *
 * This program is  free software; you can  redistribute it and/or modify it *
 * under the terms of the  GNU  General  Public  License as published by the *
 * Free  Software  Foundation; either  version 2 of the License, or (at your *
 * option) any later version.                                                *
 *                                                                           *
 * This code is distributed in the hope that it will  be useful, but WITHOUT *
 * ANY  WARRANTY; without even the  implied  warranty of  MERCHANTABILITY or *
 * FITNESS FOR A  PARTICULAR PURPOSE. See the GNU General Public License for *
 * more details.                                                             *
 *                                                                           *
 * In addition to GNU GPL this code may be used also in non GPL programs but *
 * if and  only if  programmer/distributor  of that  code  recieves  written *
 * permission from author of this code.                                      *
 *                                                                           *
 *****************************************************************************/
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
#include "common.h"
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

int action_implicit = 0;

int action_read[1000];
int action_read_pos = 0;

int action_del[1000];
int action_del_pos = 0;


int action_list = 0;
int action_write = 0;
int action_write_type = 0;
int action_send = 0;

int action_contacts = 0;
int action_contacts_cat = 0;
int action_todo = 0;
int action_todo_cat = 0;
int action_calendar = 0;

int wanted_extra_params = 0;
char **extra_params;

int action_test = 0;

int binary_mode_active = 0;

extern char alc_contacts_field_names[ALC_CONTACTS_FIELDS][20],
            alc_calendar_field_names[ALC_CALENDAR_FIELDS][20],
            alc_todo_field_names[ALC_TODO_FIELDS][20];

void help() {
    printf("This is " NAME " version " VERSION " Copyright (c) " COPYRIGHT "\n");
    printf("Usage:  %s [options] [--] [string parameters if required]\n", basename(progname));
    printf("Options:\n");
    printf("    -d<dev>/--device=<dev> ... modem device [%s]\n",default_device);
    printf("    -K<fn>/--lock=<fn> ... lock filename [/var/lock/LCK..device]\n");
    printf("    -I<s>/-init=<s> ... init string for modem [%s]\n",default_init);
    printf("    -B<n>/--rate=<n> ... baud rate [%d]\n",default_rate);
    printf("    -q/--quiet ... be more quiet\n");
    printf("    -v/--verbose ... be more verbose\n");
//    printf("    - ...  [%s]\n",default_);

    printf("Modes (at least one MUST be specified):\n");
    printf("    -i/--info ... show mobile info\n");
    printf("    -s[<n>]/--sms[=<n>] ... enable SMS mode / show selected SMS\n");
//    printf("    -c/--contacts ... list contacts\n");
    printf("    -m/--monitor ... monitor signal strength\n");
//    printf("    - \n");
    printf("    -b/--binary ... enable binary (Alcatel) mode\n");
    
    printf("Binary mode actions (at least one should be specified):\n");
    printf("    -c/--contacts ... list contacts\n");
    printf("    -a/--calendar ... list calendar entries\n");
    printf("    -t/--todo ... list todo entries\n");
    printf("    -T/--todo-cat ... list todo categories\n");
    printf("    -C/--contacts-cat ... list contacts categories\n");
	
    printf("SMS mode mode actions (in order of execution if appers more):\n");
    printf("    -x<n>/--delete=<n> ... delete message number <n>\n");
    printf("    -r<n>/--read=<n> ... read message number <n>\n");
    printf("    -w[<n>]/--write[=<n>] * ... write message as sent [default] or unsent <n>!=0\n");
    printf("    -S/--send * ... send message number <n>\n");
    printf("    -l/--list ... list all messages [default]\n");
    printf("       * = send and write require 2 parameters: phone number and message text");
//    printf("    -\n");
    
    printf("Help and simmilar:\n");
    printf("    -V/--version ... show version information\n");
    printf("    -h/--help ... this help\n");
    printf("Used symbols:\n");
    printf(" <n> = number\n");
    printf(" <dev> = device name\n");
    printf(" <fn> = file name\n");
    printf(" <s> = string\n");
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
    printf(" 203 - not enough additional parameters\n");
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
    int result, i;
    char *devname;

static const struct option longopts[]={
{"info"        ,0,0,'i'},
{"monitor"     ,0,0,'m'},
{"sms"         ,2,0,'s'},
{"binary"      ,1,0,'b'},
    
{"read"        ,1,0,'r'},
{"delete"      ,1,0,'x'},
{"list"        ,0,0,'l'},

{"write"       ,2,0,'w'},
{"send"        ,0,0,'S'},

{"contacts"    ,0,0,'c'},
{"calendar"    ,0,0,'a'},
{"todo"        ,0,0,'t'},
{"todo-cat"    ,0,0,'T'},
{"contacts-cat",0,0,'C'},

{"rate"        ,1,0,'B'},
{"init"        ,1,0,'I'},
{"lock"        ,1,0,'K'},
{"device"      ,1,0,'d'},
    
{"quiet"       ,0,0,'q'},
{"verbose"     ,0,0,'v'},

{"test"        ,0,0,'@'},

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

    while ((result=getopt_long(argc,argv,"bcatCThd:K:I:B:is::SvqVr:x:lTmw::",longopts,NULL))>0) {
        switch (result) {
            case 'w':
                wanted_extra_params += 2;
                action_write = 1;
                if (optarg != NULL) {
                    action_write_type = atoi(optarg);
                }
                break;;
            case 'S':
                wanted_extra_params += 2;
                action_send = 1;
                break;;
            case 'c':
                action_any = 1;      /* contacts can be standalone - read from sim */
                action_contacts = 1;
                break;;
            case 'a':
                action_calendar = 1;
                break;;
            case 't':
                action_todo = 1;
                break;;
            case 'C':
                action_contacts_cat = 1;
                break;;
            case 'T':
                action_todo_cat = 1;
                break;;

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
            case 'i':
                action_any = 1;
                action_info = 1;
                break;
            case 'l':
                action_list = 1;
                break;
            case 's':
                action_any = 1;
                action_sms = 1;
                if (optarg == NULL) {
                    action_implicit = 1;
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
            case 'm':
                action_monitor = 1;
                action_any = 1;
                break;
            case 'q':
                if (msg_level<MSG_NONE) msg_level++;
                break;
            case 'v':
                if (msg_level>MSG_ALL) msg_level--;
                break;
            case '@':
                action_any = 1;
                action_test = 1;
                break;
        }
        
    }
    
    if (argc - optind < wanted_extra_params) {
        message(MSG_ERROR,"Not enough addional parameters!");
        exit(203);
    } else if (argc - optind > wanted_extra_params) {
        message(MSG_ERROR,"Too much optional parameters! %d will be ignored.", argc - optind - wanted_extra_params);
    }

    if (optind < argc) {
        extra_params = malloc(MIN((argc - optind),wanted_extra_params) * sizeof(char *));
        i = 0;
        while ((optind + i)< argc) {
            extra_params[i] = argv[optind + i];
            i++;
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

void shorten_extra_params(int count) {
    wanted_extra_params -= count;
    memmove(extra_params, extra_params + count, wanted_extra_params * sizeof(char *)); 
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

void list_alc_cats(alc_type sync, alc_type type, alc_type cat) {
    int *list;
    int i;
    alc_type *result;

    alcatel_attach();

    sync_start_session();

    sync_select_type(type);
    sync_begin_read(sync);

    list = sync_get_obj_list(type, cat);
    
    message(MSG_INFO, "Received %d categories:", list[0]);

    for (i = 1; i <= list[0]; i++) {
        result = sync_get_obj_list_item(type, cat, list[i]);
        printf ("%02d: %s\n", list[i],  result);
        free(result);
    }
    
    free(list);
    
    sync_close_session(type);
    alcatel_detach();
}

void list_alc_items(alc_type sync, alc_type type) {
    alc_type *result;
    int i, j;
    int *ids, *items;
    FIELD *field;

    int count;
    
    switch (sync) {
        case ALC_SYNC_CALENDAR:
            count = ALC_CALENDAR_FIELDS;
            break;
        case ALC_SYNC_TODO:
            count = ALC_TODO_FIELDS;
            break;
        case ALC_SYNC_CONTACTS:
            count = ALC_CONTACTS_FIELDS;
            break;
    }
                
    alcatel_attach();
    
    sync_start_session();
    sync_select_type(type);
    sync_begin_read(sync);

    ids = sync_get_ids(type);

    message(MSG_INFO, "Received %d ids", ids[0]);
    
    for (i = 1; i <= ids[0]; i++) {
        message(MSG_DEBUG, "Reading id[%d] = %d", i-1, ids[i]);
        items = sync_get_fields(type, ids[i]);
        message(MSG_INFO, "Receiving data for item %d (%d fields)", ids[i], items[0]);
        printf ("Item %d (fields: %d):\n", ids[i], items[0]);
        for (j = 1; j <= items[0]; j++) {
            message(MSG_DEBUG, "items[%d] = %d", j-1, items[j]);
            result = sync_get_field_value(type, ids[i], items[j]);
            field = decode_field_value(result);
            if (items[j]  < count) {
                switch (sync) {
                    case ALC_SYNC_CALENDAR:
                        printf ("  %s:", alc_calendar_field_names[items[j]]);
                        break;
                    case ALC_SYNC_TODO:
                        printf ("  %s:", alc_todo_field_names[items[j]]);
                        break;
                    case ALC_SYNC_CONTACTS:
                        printf ("  %s:", alc_contacts_field_names[items[j]]);
                        break;
                }
            } else {
                printf(" UNKNOWN(%02d): ",items[j]);
            }
            if (field == NULL) {
                printf ("UNKNOWN TYPE (%02X %02X)\n", result[1], result[2]);
            } else {
                switch (field->type) {
                    case _date:
                        printf ("%02d. %02d. %4d\n", ((DATE *)(field->data))->day, 
                                ((DATE *)(field->data))->month, 
                                ((DATE *)(field->data))->year);
                        break;
                    case _time:
                        printf ("%02d:%02d:%02d\n", ((TIME *)(field->data))->hour, 
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
    
    sync_close_session(type);
    alcatel_detach();
}

void print_message(SMS* sms) {
    if (((*sms).stat == SMS_SENT) || ((*sms).stat == SMS_UNSENT)) {
        printf("To: %s\nSMSC: %s\nStatus: %d\nPosition: %d\n\n%s\n",
            (*sms).sendr,
            (*sms).smsc,
            (*sms).stat,
            (*sms).pos,
            (*sms).ascii);
    } else {
        printf("From: %s\nDate: %sSMSC: %s\nStatus: %d\nPosition: %d\n\n%s\n",
            (*sms).sendr,
            ctime(&((*sms).date)),
            (*sms).smsc,
            (*sms).stat,
            (*sms).pos,
            (*sms).ascii);
    }
}

void list_messages() {
    SMS *sms;
    int i;
    sms = get_smss(SMS_ALL);
    i = 0;
    while (sms[i].pos != -1) {
        print_message(sms+i);
        i++;
        if (sms[i].pos != -1) printf ("-------------------------------------------------------------------------------\n");
    }
    free(sms);
}

void read_messages() {
    SMS *sms;
    int i;
    for (i = 0; i < action_read_pos; i++){
        sms = get_sms(action_read[i]);
        if (sms) {
            print_message(sms);
            free(sms);
        } else {
            message(MSG_ERROR,"Message %d not found!", action_read[i]);
        }
        if (i != action_read_pos - 1) printf ("-------------------------------------------------------------------------------\n");
    }
}

void delete_messages() {
    int i;
    for (i = 0; i < action_del_pos; i++){
        if (delete_sms(action_del[i])) {
            message(MSG_INFO,"Message %d deleted!", action_del[i]);
        } else {
            message(MSG_ERROR,"Message %d not found!", action_del[i]);
        }
    }
}

void send_message() {
    char pdu[1024];
    int i;
    
    make_pdu(extra_params[0], extra_params[1], 0, PDU_CLASS_SIM, pdu);
    i = send_sms(pdu);
    if (i != -1) printf ("Message send, message reference = %d\n", i);
    else message(MSG_ERROR, "Message not sent!");
    shorten_extra_params(2);
}

void write_message() {
    char pdu[1024];
    int i;

    make_pdu(extra_params[0], extra_params[1], 0, PDU_CLASS_SIM, pdu);
    i = put_sms(pdu, (action_write_type == 0) ? SMS_SENT : SMS_UNSENT);
    if (i != -1) printf ("Message written at position %d\n", i);
    else message(MSG_ERROR, "Message not written!");
    shorten_extra_params(2);
}

void test() {
    printf ("Currently no feature to test!\n");
}

int main(int argc, char *argv[]) {
    char data[1024];
    char *s;
    int i, j;

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

        s = get_smsc();
        printf ("SMS centre number: %s\n", s);
    }

    if (action_sms) {
        if (action_del_pos > 0) {
            delete_messages();
            action_implicit = 0;
        }
        if (action_read_pos > 0) {
            read_messages();
            action_implicit = 0;
        }
        if (action_write) {
            write_message();
            action_implicit = 0;
        }
        if (action_send) {
            send_message();
            action_implicit = 0;
        }
        if (action_list || action_implicit) list_messages();
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

        if (action_todo_cat) list_alc_cats(ALC_SYNC_TODO, ALC_SYNC_TYPE_TODO, ALC_LIST_TODO_CAT);
        if (action_todo) list_alc_items(ALC_SYNC_TODO, ALC_SYNC_TYPE_TODO);
        if (action_contacts_cat) list_alc_cats(ALC_SYNC_CONTACTS, ALC_SYNC_TYPE_CONTACTS, ALC_LIST_CONTACTS_CAT);
        if (action_contacts) list_alc_items(ALC_SYNC_CONTACTS, ALC_SYNC_TYPE_CONTACTS);
        if (action_calendar) list_alc_items(ALC_SYNC_CALENDAR, ALC_SYNC_TYPE_CALENDAR);
        
        alcatel_done();
        binary_mode_active = 0;
        
    }

    if (action_test) {
		test();
    }

    return 0;
}
