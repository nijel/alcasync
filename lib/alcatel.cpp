/*
 * alcasync/alcatel.cpp
 *
 * low level functions for communication with Alcatel One Touch 501 and
 * compatible mobile phone
 *
 * NOTE:
 * This is EXPERIMANTAL implementation of comunication protocol used by
 * Alcatel 501 (probably also any 50x and 70x) mobile phone. This code may
 * destroy your phone, so use it carefully. However whith my phone works this
 * code correctly. This code assumes following conditions:
 *  - no packet is lost
 *  - 0x0F ack doesn't mean anything important
 *  - data will be received as they are expected
 *  - no error will appear in transmission
 *  - all string are given in form that works in mobile and also in C
 *  - all magic numbers mean that, what I thing that they mean ;-)
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
 * if and only if programmer/distributor of that code receives written
 * permission from author of this code.
 *
 */
/* $Id$ */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "modem.h"
#include "alcatel.h"
#include "logging.h"
#include "common.h"
#include "charset.h"

#define SLEEP_CHANGE    10000
#define SLEEP_FAIL      200

int alcatel_errno;

char alc_contacts_field_names[ALC_CONTACTS_FIELDS][20] = {
    "LastName",
    "FirstName",
    "Company",
    "JobTitle",
    "Note",
    "Category",
    "Private",
    "WorkNumber",
    "MainNumber",
    "FaxNumber",
    "OtherNumber",
    "PagerNumber",
    "MobileNumber",
    "HomeNumber",
    "Email1",
    "Email2",
    "Address",
    "City",
    "State",
    "Zip",
    "Country",
    "Custom1",
    "Custom2",
    "Custom3",
    "Custom4"
};

char alc_calendar_field_names[ALC_CALENDAR_FIELDS][20] = {
    "Date",
    "StartTime",
    "EndTime",
    "AlarmDate",
    "AlarmTime",
    "Subject",
    "Private",
    "EventType",
    "ContactID",
    "PhoneNumber",          /* Accesible by IntelliSync, but not in mobile */
    "DayOfWeek",
    "Day",
    "WeekOfMonth",
    "Month",
    "UNKNOWN (14)",         /* I haven't seen this yet */
    "UNKNOWN (15)",         /* I haven't seen this yet */
    "UNKNOWN (16)",         /* I haven't seen this yet */
    "Frequency",
    "StartDate",
    "StopDate",
    /* Following two used when EventType is alarm (and Intellisync sometimes
       creates them, but when it reads them it fails) */
    "AlarmDate2",
    "AlarmTime2"
};

char alc_todo_field_names[ALC_TODO_FIELDS][20] = {
    "DueDate",
    "Completed",
    "AlarmDate",
    "AlarmTime",
    "Subject",
    "Private",
    "Category",
    "Priority",
    "ContactID",
    "PhoneNumber"           /* Accesible by IntelliSync, but not in mobile */
    /* DATE, probably also Alarm */
    /* TIME, probably also Alarm */
};

alc_type in_counter = 0;
alc_type out_counter = 0;

alc_type recv_buffer[1024];
int recv_buffer_pos = 0;

void alcatel_send_packet(alc_type type, alc_type *data, alc_type len) {
    static alc_type buffer[1024];
    int size = 0, i, checksum = 0;
    buffer[0] = 0x7E;
    buffer[1] = type;
    switch (type) {
        case ALC_CONNECT:
            buffer[2] = 0x0A;
            buffer[3] = 0x04;
            buffer[4] = 0x00;
            size = 5;
            break;
        case ALC_DISCONNECT:
            size = 2;
            break;
        case ALC_DATA:
            buffer[2] = out_counter;
            if (out_counter == 0x3D) out_counter = 0;
            else out_counter++;
            buffer[3] = '\0';
            buffer[4] = len;
            memcpy(buffer+5, data, len);
            size = 5 + len;
            break;
        case ALC_ACK:
            buffer[2] = in_counter;
            size = 3;
            break;
    }
    for (i=0; i<size; i++)
        checksum ^= buffer[i];
    buffer[size] = checksum;
    size ++;
    message(MSG_DEBUG,"Sending packet %s", hexdump(buffer, size, 1));
    modem_send_raw(buffer, size); 
}

int alcatel_recv_data(int size) {
    int result, fails = 0;
    message(MSG_DEBUG2,"We have %d bytes, we want %d bytes", recv_buffer_pos, size);
    while (recv_buffer_pos < size) { 
        result = modem_read_raw(recv_buffer + recv_buffer_pos, sizeof(recv_buffer) - recv_buffer_pos);
        if (result > 0) {
            recv_buffer_pos += result;
            message(MSG_DEBUG2,"Received raw data %s", hexdump(recv_buffer, recv_buffer_pos, 1));
            fails = 0;
        } else {
            message(MSG_DEBUG2,"Receive failed (%d): %s", result, strerror(errno));
            usleep(SLEEP_FAIL);
            fails ++;
            if (fails > 200) {
                message(MSG_ERROR,"Reading failed...");
                return false;
            }
        }
    }
    return true;
}

void alcatel_recv_shorten(int size) {
    recv_buffer_pos -= size;
    memmove(recv_buffer, recv_buffer + size, recv_buffer_pos);
}

alc_type *alcatel_recv_packet(bool ack) {
    alc_type *data;
    alc_type num;
    alc_type size;
    int wanted_size;

    if (!alcatel_recv_data(5)) return NULL; /* 5 is minimal size of packet */
    
    if (recv_buffer[0] != 0x7e)
        message(MSG_ERROR, "Bad data? %s", hexdump(recv_buffer, recv_buffer_pos, 1));

    message(MSG_DEBUG2,"Packet type %02X", recv_buffer[1]);
    
    num = recv_buffer[2];
    if (num == 0x3D) in_counter = 0;
    else in_counter = num + 1;
    size = recv_buffer[4];
    wanted_size = size + 6;

    if (!alcatel_recv_data(wanted_size)) return NULL;
    
    message(MSG_DEBUG,"Received packet %s", hexdump(recv_buffer, wanted_size, 1));

    data = (alc_type *)malloc(wanted_size);
    chk(data);
    memcpy(data, recv_buffer, wanted_size);

    alcatel_recv_shorten(wanted_size);

    /* here should be checked checksum, but I don't know what to do if it fails ;-) */

    if (ack)
        alcatel_send_packet(ALC_ACK, 0, 0);

    return data;
}

alc_type *alcatel_recv_ack(alc_type type){
    alc_type *data;
    int wanted_size;
    int once_again = 0;

    if (!alcatel_recv_data(3)) return NULL; /* 3 is minimal size of ack */

    if (recv_buffer[0] != 0x7e)
        message(MSG_ERROR, "Bad data? %s", hexdump(recv_buffer, recv_buffer_pos, 1));
    
    message(MSG_DEBUG2,"Ack type %02X", recv_buffer[1]);
    
    switch (recv_buffer[1]) {
        case ALC_DISCONNECT_ACK:
            message(MSG_DEBUG2,"Disconnect ack - throwing away");
            wanted_size = 3;
            break;
        case ALC_CONNECT_ACK:
            message(MSG_DEBUG2,"Connect ack - throwing away");
            wanted_size = 6;
            break;
        case ALC_CONTROL_ACK:
            message(MSG_DEBUG2,"Control ack - don't know what it means - throwing away");
            wanted_size = 4;
            /* this is some magic ;-) */
            if (type == ALC_ACK) once_again = 1;
            break;
        case ALC_ACK:
            message(MSG_DEBUG2,"Normal ack - throwing away");
            wanted_size = 4;
            break;
        case ALC_DATA:
            message(MSG_WARNING,"Received data instead of ack -> we ignore it and resend ack of that data");
            /* probably mobile didn't receive ack */
            free(alcatel_recv_packet(1));
            return alcatel_recv_ack(type);
            break;
        default:
            message(MSG_ERROR,"Unknown ack! (%02X)", recv_buffer[1]);
            wanted_size = 4;
    }

    if (recv_buffer[1] != type && recv_buffer[1] != ALC_CONTROL_ACK)
            message(MSG_WARNING,"Received another ack than expected! (recv: %02X, expect: %02X)", recv_buffer[1], type);

    if (!alcatel_recv_data(wanted_size)) return NULL;
    
    message(MSG_DEBUG,"Received ack %s", hexdump(recv_buffer, wanted_size, 1));

    if (once_again) {
        alcatel_recv_shorten(wanted_size);
        return alcatel_recv_ack(type);
    } else {
        data = (alc_type *)malloc(wanted_size);
        chk(data);
        memcpy(data, recv_buffer, wanted_size);

        alcatel_recv_shorten(wanted_size);
        
        /* here should be checked checksum, but I don't know what to do if it fails ;-) */

        return data;
    }
}

bool alcatel_init(void){
    char answer[500];
    alc_type *data;

    message(MSG_DETAIL,"Entering Alcatel binary mode");
    modem_cmd("AT+IFC=2,2\r",answer,sizeof(answer),100,0);
    modem_cmd("AT+CPROT=16,\"V1.0\",16\r",answer,sizeof(answer),100,"CONNECT");
    modem_flush();
    message(MSG_DETAIL,"Alcatel binary mode started");

    alcatel_send_packet(ALC_CONNECT,0,0);
    data = alcatel_recv_ack(ALC_CONNECT_ACK);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }
    free(data);
    usleep(SLEEP_CHANGE);
    return true;
}

bool alcatel_attach(void){
    alc_type *data;
    alc_type buffer[] = {0x00,0x00,0x7C,0x20};

    message(MSG_DETAIL, "Attaching to Alcatel");
    alcatel_send_packet(ALC_DATA, buffer, 4); 
    free(alcatel_recv_ack(ALC_ACK));
    data = alcatel_recv_packet(1);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }
    free(data);
    message(MSG_DETAIL,"Attached to Alcatel");
    return true;
}

bool alcatel_detach(void){
    alc_type *data;
    alc_type buffer[] = {0x00,0x01,0x7C,0x00};

    message(MSG_DETAIL, "Detaching from Alcatel");
    alcatel_send_packet(ALC_DATA, buffer, 4); 
    free(alcatel_recv_ack(ALC_ACK));
    data = alcatel_recv_packet(1);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }
    free(data);
    message(MSG_DETAIL, "Detached from Alcatel");
    return true;
}

bool alcatel_done(void){
    alc_type *data;

    message(MSG_DETAIL,"Leaving Alcatel binary mode");
    alcatel_send_packet(ALC_DISCONNECT,0,0);
    data = alcatel_recv_ack(ALC_DISCONNECT_ACK);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }
    free(data);
    message(MSG_DETAIL,"Alcatel binary mode left");
    usleep(SLEEP_CHANGE);
    return true;
}

bool alcatel_start_session(void) {
    alc_type *data;
    alc_type buffer[] = {0x00, 0x04, 0x7C, 0x80, 0x12, 0x34, 0x56, 0x78};
//                                               ^^^^^^^^^^^^^^^^^^^^^^ - this is DBID but it was always same...

    message(MSG_DETAIL,"Starting alcatel session");
    alcatel_send_packet(ALC_DATA, buffer, 8);
    free(alcatel_recv_ack(ALC_ACK));
    data = alcatel_recv_packet(1);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }
    free(data);
    return true;
}

bool alcatel_close_session(alc_type type) {
    alc_type *data;
    alc_type buffer[] = {0x00, 0x04, type, 0x23, 0x01};
//                                                      ^^^^
// this is session id, but this software currently supports only one session...
    
    message(MSG_DETAIL,"Closing alcatel session");
    alcatel_send_packet(ALC_DATA, buffer, 5);
    free(alcatel_recv_ack(ALC_ACK));
    data = alcatel_recv_packet(1);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }
    free(data);
    return true;
}

bool alcatel_select_type(alc_type type) {
    alc_type buffer[] = {0x00, 0x00, type, 0x20};
    alc_type buffer2[] = {0x00, 0x04, type, 0x22, 0x01, 0x00};

    bool result;
    alc_type *answer;
    
    message(MSG_DETAIL,"Setting sync type: %02X", type);

    alcatel_send_packet(ALC_DATA, buffer, 4); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    
    alcatel_send_packet(ALC_DATA, buffer2, 6); 
    free(alcatel_recv_ack(ALC_ACK));

    answer = alcatel_recv_packet(1);
    if (answer == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }

    result = (answer[8] == 0);
    
    free(answer);

    if (result)
        free(alcatel_recv_packet(1)); //[9] = session ID
    else
        alcatel_errno = answer[8];

    return result;
}

bool alcatel_begin_transfer(alc_type type) {
    alc_type *data;
    alc_type buffer[] = {0x00, 0x04, 0x7C, 0x81, type, 0x00, 0x85, 0x00};
    
    message(MSG_DETAIL, "Starting transfer");
    alcatel_send_packet(ALC_DATA, buffer, 8);
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (data == NULL) return false;
    free(data);
    return true;
}

int *alcatel_get_ids(alc_type type) {
    alc_type buffer[] = {0x00, 0x04, type, 0x2F, 0x01};
    alc_type *data;
    int *result = NULL;
    int count = 0,size =0, i, pos;
    bool isLast = false;
    
    message(MSG_DETAIL, "Reading items list");
    alcatel_send_packet(ALC_DATA, buffer, 5);
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));

    while (!isLast) {
        data = alcatel_recv_packet(1);
        if (data==NULL) {
            alcatel_errno = ALC_ERR_DATA;
            return NULL;
        }

        count = data[10];
        size += count;

        result = (int *)realloc(result, (size + 1)* sizeof(int));
        chk(result);

        result[0] = size;

        for (i = 0; i < count; i++) {
            pos = 11 + (4 * i);
            result[size - count + i + 1] = data[pos + 3] + (data[pos + 2] << 8) + (data[pos + 1] << 16) + (data[pos] << 24);
        }

        isLast = data[4 + data[4]] == 0;

        free(data);
    }

    return result;
}

int *alcatel_get_fields(alc_type type, int item) {
    alc_type buffer[] = {0x00, 0x04, type, 0x30, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff)};
    alc_type *data;
    int *result;
    int count = 0, i;
    
    message(MSG_DETAIL, "Reading item fields (%d)", item);
    alcatel_send_packet(ALC_DATA, buffer, 9);
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (data==NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return NULL;
    }

    count = data[14];

    result = (int *)malloc((count + 1)* sizeof(int));
    chk(result);

    result[0] = count;

    for (i = 0; i < count; i++) {
        result[i + 1] = data[15 + i];
    }

    return result;
}

alc_type *alcatel_get_field_value(alc_type type, int item, int field) {
    alc_type buffer[] = {0x00, 0x04, type, 0x1f, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff), (field & 0xff)};
    alc_type *data;
    alc_type *result;
    int len;

    message(MSG_DETAIL, "Reading item value (%08x.%02x)", item, field);
    alcatel_send_packet(ALC_DATA, buffer, 10);
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (data==NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return NULL;
    }

    len = data[4] - 12;

    result = (alc_type *)malloc(len + 1);
    chk(result);

    result[0] = len;

    memcpy(result + 1,data + 17, len);
    
    return result;
}

AlcatelFieldStruct *decode_field_value(alc_type *buffer) {
    AlcatelDateStruct *date;
    AlcatelTimeStruct *time;
    AlcatelFieldStruct *field;
    alc_type *s;
    int *i;
    int j;

    field = (AlcatelFieldStruct *)malloc(sizeof(AlcatelFieldStruct));
    chk(field);
    
    if (buffer[1] == 0x05 && buffer[2] == 0x67) {
        /* date */
        date = (AlcatelDateStruct *)malloc(sizeof(AlcatelDateStruct));
        chk(date);
        date->day = buffer[4];
        date->month = buffer[5];
        date->year = buffer[7] + (buffer[6] << 8); 

        field->type = _date;
        field->data = date;
    } else if (buffer[1] == 0x06 && buffer[2] == 0x68) {
        /* time */
        time = (AlcatelTimeStruct *)malloc(sizeof(AlcatelTimeStruct));
        chk(time);
        time->hour = buffer[4];
        time->minute = buffer[5];
        time->second = buffer[6];

        field->type = _time;
        field->data = time;
    } else if (buffer[1] == 0x08 && buffer[2] == 0x3C) {
        /* string */
        s = (alc_type *)malloc(buffer[3]+1);
        chk(s);
        memcpy(s, buffer+4, buffer[3]);
        s[buffer[3]] = 0;

        for (j=0; j<buffer[3]; j++) s[j] = gsm2ascii(s[j]);
        
        field->type = _string;
        field->data = s;
    } else if (buffer[1] == 0x07 && buffer[2] == 0x3C) {
        /* phone */
        s = (alc_type *)malloc(buffer[3]+1);
        chk(s);
        memcpy(s, buffer+4, buffer[3]);
        s[buffer[3]] = 0;
        
        for (j=0; j<buffer[3]; j++) s[j] = gsm2ascii(s[j]);

        field->type = _phone;
        field->data = s;
    } else if (buffer[1] == 0x03 && buffer[2] == 0x3B) {
        /* boolean */
        i = (int *)malloc(sizeof(int));
        chk(i);
        *i = buffer[3];

        field->type = _bool;
        field->data = i;
    } else if (buffer[1] == 0x02 && buffer[2] == 0x3A) {
        /* integer */
        i = (int *)malloc(sizeof(int));
        chk(i);
        *i = buffer[6] + (buffer[5] << 8) + (buffer[4] << 16) + (buffer[3] << 24);

        field->type = _int;
        field->data = i;
    } else if (buffer[1] == 0x04 && buffer[2] == 0x38) {
        /* enumeration */
        i = (int *)malloc(sizeof(int));
        chk(i);
        *i = buffer[3];

        field->type = _enum;
        field->data = i;
    } else if (buffer[1] == 0x00 && buffer[2] == 0x38) {
        /* byte */
        i = (int *)malloc(sizeof(int));
        chk(i);
        *i = buffer[3];

        field->type = _byte;
        field->data = i;
    } else {
        free(field);
        return NULL;
    }
    return field;

}

int *alcatel_get_obj_list(alc_type type, alc_type list) {
    alc_type buffer[] = {0x00, 0x04, type, 0x0b, list};
    alc_type *data;
    int *result;
    int count = 0, i;
    
    message(MSG_DETAIL, "Reading list items");
    alcatel_send_packet(ALC_DATA, buffer, 5);
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (data==NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return NULL;
    }

    if (data[4] < 8) count = 0;
    else count = data[12];

    result = (int *)malloc((count + 1)* sizeof(int));
    chk(result);

    result[0] = count;

    for (i = 0; i < count; i++) {
        result[i + 1] = data[13 + i];
    }
    free(data);

    return result;
}

char *alcatel_get_obj_list_item(alc_type type, alc_type list, int item) {
    alc_type buffer[] = {0x00, 0x04, type, 0x0c, list, 0x0A, 0x01, (item & 0xff) };
    alc_type *data;
    char *result;
    int len;
    int j;

    message(MSG_DETAIL, "Reading list item name (%d)", item);

    alcatel_send_packet(ALC_DATA, buffer, 8);
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (data==NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return NULL;
    }

    len = data[14]; 

    result = (char *)malloc(len + 1);
    chk(result);

    memcpy(result,data + 15, len);

    for (j=0; j<=len; j++) result[j] = gsm2ascii(result[j]);

    free(data);
    
    result[len] = 0;
    
    return result;
}

bool alcatel_del_obj_list_items(alc_type type, alc_type list) {
    alc_type *data;
    alc_type buffer[] = {0x00, 0x04, type, 0x0e, list};

    message(MSG_DETAIL, "Deleting list items");

    alcatel_send_packet(ALC_DATA, buffer, 5);
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }
    free(data);
    return true;
}

int alcatel_create_obj_list_item(alc_type type, alc_type list, const char *item) {
    alc_type buffer[256] = {0x00, 0x04, type, 0x0d, list, 0x0b };
    alc_type *data;
    int i;

    message(MSG_DETAIL, "Creating list item (%s)", item);

    i = strlen(item);
    buffer[6] = (alc_type)(i + 1);
    buffer[7] = (alc_type)i;
    memcpy(buffer + 8, item, i);

    alcatel_send_packet(ALC_DATA, buffer, 8 + i); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (data == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return -1;
    }

    i = data[12]; 
    free(data);
    return i;
}

int alcatel_commit(alc_type type) {
    alc_type buffer[] = {0x00, 0x04, type, 0x20, 0x01 };
//                                               ^^^^
// this is session id, but this software currently supports only one session...
    alc_type *data;
    int result;

    message(MSG_DETAIL, "Commiting record");

    alcatel_send_packet(ALC_DATA, buffer, 5); 
    free(alcatel_recv_ack(ALC_ACK));
    data = alcatel_recv_packet(1);
    if (data[8] == 0) {
        free(data);
        data = alcatel_recv_packet(1);
        result = data[12] + (data[11] << 8) + (data[10] << 16) + (data[9] << 24);
//      7E 02 15 00 09 00 07 68 20 00 00 00 37 00 18
//      7E 02 0A 00 09 00 07 68 20 00 00 00 58 00 68
//                                 ^^^^^^^^^^^ should be returned....
        free(data);
    } else {
        alcatel_errno = ALC_ERR_DATA;
        result = -1;
        free(data);
    }
    return result;
}

bool alcatel_update_field(alc_type type, int item, int field, AlcatelFieldStruct *data) {
    alc_type buffer[180] = {0x00, 0x04, type, 0x26, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff),
        0x65, 0x00 /* length of remaining part */, (field & 0xff), 0x37 /* here follows data */};
    alc_type *answer;
    bool result;
    int j;
    
    if (data->data == NULL) {
        return alcatel_delete_field(type, item, field);
    }

    message(MSG_DETAIL, "Updating field (%08x.%02x)", item, field);

    switch (data->type) {
        case _date:
            buffer[13] = 0x05;
            buffer[14] = 0x67;
            
            buffer[10] = 0x09;
            buffer[15] = 0x04;
            buffer[16] = ((AlcatelDateStruct *)(data->data))->month;
            buffer[17] = ((AlcatelDateStruct *)(data->data))->day;
            buffer[18] = ((AlcatelDateStruct *)(data->data))->year >> 8;
            buffer[19] = ((AlcatelDateStruct *)(data->data))->year & 0xff;
            buffer[20] = 0x00;
            break;
        case _time:
            buffer[13] = 0x06;
            buffer[14] = 0x68;

            buffer[10] = 0x08;
            buffer[15] = 0x03;
            buffer[16] = ((AlcatelTimeStruct *)(data->data))->hour;
            buffer[17] = ((AlcatelTimeStruct  *)(data->data))->minute;
            buffer[18] = ((AlcatelTimeStruct  *)(data->data))->second;
            buffer[19] = 0x00;
            break;
        case _string:
            buffer[13] = 0x08;
            buffer[14] = 0x3c;

            strncpy((char *)(buffer + 16), (char *)(data->data), 150); /* maximally 150 chars */
            buffer[15] = strlen((char *)(buffer + 16));
            for (j=0; j<=buffer[15]; j++) buffer[16 + j] = ascii2gsm(buffer[16 + j]);
            buffer[10] = 5 + buffer[15];
            buffer[16 + buffer[15]] = 0x00;
            break;
        case _phone:
            buffer[13] = 0x07;
            buffer[14] = 0x3c;

            strncpy((char *)(buffer + 16), (char *)(data->data), 150); /* maximally 150 chars, maybe here is another limitation... */
            buffer[15] = strlen((char *)(buffer + 16));
            for (j=0; j<=buffer[15]; j++) buffer[16 + j] = ascii2gsm(buffer[16 + j]);
            buffer[10] = 5 + buffer[15];
            buffer[16 + buffer[15]] = 0x00;
            break;
        case _enum:
            buffer[13] = 0x04;
            buffer[14] = 0x38;

            buffer[10] = 0x05;
            buffer[15] = *(int *)(data->data) & 0xff;
            buffer[16] = 0x00;
            break;
        case _bool:
            buffer[13] = 0x03;
            buffer[14] = 0x3b;

            buffer[10] = 0x05;
            buffer[15] = *(int *)(data->data) & 0xff;
            buffer[16] = 0x00;
            break;
        case _int:
            buffer[13] = 0x02;
            buffer[14] = 0x3a;

            buffer[10] = 0x08;
            buffer[15] = *(int *)(data->data) >> 24;
            buffer[16] = (*(int *)(data->data) >> 16) & 0xff;
            buffer[17] = (*(int *)(data->data) >> 8) & 0xff;
            buffer[18] = *(int *)(data->data) & 0xff;
            buffer[19] = 0x00;
            break;
        case _byte:
            buffer[13] = 0x00;
            buffer[14] = 0x38;

            buffer[10] = 0x05;
            buffer[15] = *(int *)(data->data) & 0xff;
            buffer[16] = 0x00;
            break;
    }

    alcatel_send_packet(ALC_DATA, buffer, 12 + buffer[10]); 
    free(alcatel_recv_ack(ALC_ACK));
    answer = alcatel_recv_packet(1);
    if (answer == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }

    if (!(result = (answer[8] == 0)))
        alcatel_errno = answer[8];

    free(answer);

    return result;
}

bool alcatel_create_field(alc_type type, int field, AlcatelFieldStruct *data) {
    alc_type buffer[180] = {0x00, 0x04, type, 0x25, 0x01, 0x65, 0x00 /* length of remaining part */, (field & 0xff), 0x37 /* here follows data */};
    alc_type *answer;
    bool result;
    
    if (data->data == NULL) {
        // no need to create empty field
        return true;
    }

    message(MSG_DETAIL, "Creating field (????.%02x)", field);

    switch (data->type) {
        case _date:
            buffer[9] = 0x05;
            buffer[10] = 0x67;

            buffer[6] = 0x09;
            buffer[11] = 0x04;
            buffer[12] = ((AlcatelDateStruct *)(data->data))->month;
            buffer[13] = ((AlcatelDateStruct *)(data->data))->day;
            buffer[14] = ((AlcatelDateStruct *)(data->data))->year >> 8;
            buffer[15] = ((AlcatelDateStruct *)(data->data))->year & 0xff;
            buffer[16] = 0x00;
            break;
        case _time:
            buffer[9] = 0x06;
            buffer[10] = 0x68;

            buffer[6] = 0x08;
            buffer[11] = 0x03;
            buffer[12] = ((AlcatelTimeStruct *)(data->data))->hour;
            buffer[13] = ((AlcatelTimeStruct *)(data->data))->minute;
            buffer[14] = ((AlcatelTimeStruct *)(data->data))->second;
            buffer[15] = 0x00;
            break;
        case _string:
            buffer[9] = 0x08;
            buffer[10] = 0x3c;

            strncpy((char *)(buffer + 12), (char *)(data->data), 150); /* maximally 150 chars */
            buffer[11] = strlen((char *)(buffer + 12));
            buffer[6] = 5 + buffer[11];
            buffer[12 + buffer[11]] = 0x00;
            break;
        case _phone:
            buffer[9] = 0x07;
            buffer[10] = 0x3c;

            strncpy((char *)(buffer + 12), (char *)(data->data), 150); /* maximally 150 chars, maybe here is another limitation... */
            buffer[11] = strlen((char *)(buffer + 12));
            buffer[6] = 5 + buffer[11];
            buffer[12 + buffer[11]] = 0x00;
            break;
        case _enum:
            buffer[9] = 0x04;
            buffer[10] = 0x38;

            buffer[6] = 0x05;
            buffer[11] = *(int *)(data->data) & 0xff;
            buffer[12] = 0x00;
            break;
        case _bool:
            buffer[9] = 0x03;
            buffer[10] = 0x3b;

            buffer[6] = 0x05;
            buffer[11] = *(int *)(data->data) & 0xff;
            buffer[12] = 0x00;
            break;
        case _int:
            buffer[9] = 0x02;
            buffer[10] = 0x3a;

            buffer[6] = 0x08;
            buffer[11] = *(int *)(data->data) >> 24;
            buffer[12] = (*(int *)(data->data) >> 16) & 0xff;
            buffer[13] = (*(int *)(data->data) >> 8) & 0xff;
            buffer[14] = *(int *)(data->data) & 0xff;
            buffer[15] = 0x00;
            break;
        case _byte:
            buffer[9] = 0x00;
            buffer[10] = 0x38;

            buffer[6] = 0x05;
            buffer[11] = *(int *)(data->data) & 0xff;
            buffer[12] = 0x00;
            break;
    }

    alcatel_send_packet(ALC_DATA, buffer, 8 + buffer[6]); 
    free(alcatel_recv_ack(ALC_ACK));
    answer = alcatel_recv_packet(1);
    if (answer == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }

    if (!(result = (answer[8] == 0)))
        alcatel_errno = answer[8];

    free(answer);

    return result;
}

bool alcatel_delete_item(alc_type type, int item) {
    alc_type buffer[] = {0x00, 0x04, type, 0x27, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff), 0x42};
    alc_type *answer;
    bool result;

    message(MSG_DETAIL, "Deleting item (%08x)", item);

    alcatel_send_packet(ALC_DATA, buffer, 10); 
    free(alcatel_recv_ack(ALC_ACK));
    answer = alcatel_recv_packet(1);
    if (answer == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }

    if (!(result = (answer[8] == 0)))
        alcatel_errno = answer[8];

    free(answer);
    free(alcatel_recv_packet(1));

    return result;
}

bool alcatel_delete_field(alc_type type, int item, int field) {
    alc_type buffer[] = {0x00, 0x04, type, 0x26, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff), 0x65, 0x01, field & 0xff, 0x01};
    alc_type *answer;
    bool result;

    message(MSG_DETAIL, "Deleting field (%08x.%02x)", item, field);

    alcatel_send_packet(ALC_DATA, buffer, 13); 
    free(alcatel_recv_ack(ALC_ACK));
    answer = alcatel_recv_packet(1);
    if (answer == NULL) {
        alcatel_errno = ALC_ERR_DATA;
        return false;
    }

    if (!(result = (answer[8] == 0)))
        alcatel_errno = answer[8];

    free(answer);

    return result;
}

/* TODO:

alc>write 00067c81
Sending packet: 00067c81
Receiving ack
alc>read
Received data:
7E 0204 0009 0046 7C00 8112 3456 78C2
                         ^^^^^^^^^^ > DBID

alc>write 00067c80
Sending packet: 00067c80
Receiving ack
alc>read
Received data:
7E 0205 0009 0046 7C00 80FF FFFF FFCA
>> InteliSync:
7E 0203 0009 0046 7C00 8000 0005 CE07 
                              ^^^^^ > device ID????

"timestapms":
                              
09/30 10:24:35.189  Clt : **** MInvoke-MAction SyncBeginSynchro(SectType = 2, bImport = 0, SyncMode = 1, SyncStamp = 1033377559)

1033377559 = 0x3D981717 = Sep 30 10:19:19 CET 2002
09/30 10:24:35.189  Clt : Packet #11 sent : 19 bytes :
	7E 02 04 00 0D 00 04 7C 81 02 00 85 01 86 3D 98 ~      |      = 
	17 17 29                                          )
	
09/30 10:24:35.219  Clt : **** Sending Ack to #100014, Ns 0x6, Nr 0x7
09/30 10:24:35.219  Clt : Packet #12 sent : 4 bytes :
	7E 06 07 7F                                     ~   
	
09/30 10:24:35.229  Clt : PLP_ReceivePacket() #100014 (Q size after read 0) : 11 bytes :
	7E 02 06 00 05 00 44 7C 00 81 C6                ~     D|   
	
09/30 10:24:35.239  Clt : **** Sending Ack to #100015, Ns 0x7, Nr 0x8
09/30 10:24:35.239  Clt : Packet #13 sent : 4 bytes :
	7E 06 08 70                                     ~  p
	
09/30 10:24:35.249  Clt : PLP_ReceivePacket() #100015 (Q size after read 0) : 13 bytes :
	7E 02 07 00 07 00 07 7C 80 00 85 01 03          ~      |     
	
09/30 10:24:35.249  Clt : **** MInvoke-MAction UDSSyncGetChangeList(SessionID = 1)
09/30 10:24:35.249  Clt : Packet #14 sent : 11 bytes :
	7E 02 05 00 05 00 04 68 2E 01 3F                ~      h. ?
	
09/30 10:24:35.289  Clt : **** Sending Ack to #100018, Ns 0x8, Nr 0x9
09/30 10:24:35.299  Clt : Packet #15 sent : 4 bytes :
	7E 06 09 71                                     ~  q
	
09/30 10:24:35.299  Clt : PLP_ReceivePacket() #100018 (Q size after read 0) : 11 bytes :
	7E 02 08 00 05 00 44 68 00 2E 73                ~     Dh .s
	
09/30 10:24:35.309  Clt : **** Sending Ack to #100019, Ns 0x9, Nr 0xA
09/30 10:24:35.309  Clt : Packet #16 sent : 4 bytes :
	7E 06 0A 72                                     ~  r
	
09/30 10:24:35.319  Clt : PLP_ReceivePacket() #100019 (Q size after read 0) : 13 bytes :
	7E 02 09 00 07 00 07 68 27 6A 00 00 50          ~      h'j  P
	

end of sync:
[...]
09/30 10:24:55.207  Clt : **** MInvoke-MAction SyncEndSynchro(SectType = 2, SyncStamp = 1033377862)
09/30 10:24:55.207  Clt : Packet #46 sent : 15 bytes :
	7E 02 10 00 09 00 04 7C 82 02 3D 98 18 46 66    ~      |  =  Ff
	
09/30 10:24:55.247  Clt : **** Sending Ack to #100055, Ns 0x1D, Nr 0x1E
09/30 10:24:55.247  Clt : Packet #47 sent : 4 bytes :
	7E 06 1E 66                                     ~  f
	
09/30 10:24:55.247  Clt : PLP_ReceivePacket() #100055 (Q size after read 0) : 11 bytes :
	7E 02 1D 00 05 00 44 7C 00 82 DE                ~     D|   
	
09/30 10:24:55.247  Clt : ++++ MInvoke-MAction SyncEndSynchro(SectType = 2, SyncStamp = 1033377862) returned 0

*/
