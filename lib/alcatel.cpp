/*
 * alcatool/alcatel.cpp
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
 *  - data will be recieved as they are expected
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
 * if and only if programmer/distributor of that code recieves written
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
#define SLEEP_FAIL      100

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
    "KNOWN UNKNOWN (9)",     /* I haven't seen this yet */
    "DayOfWeek",
    "Day",
    "WeekOfMonth",
    "Month",
    "KNOWN UNKNOWN (14)",    /* I haven't seen this yet */
    "KNOWN UNKNOWN (15)",    /* I haven't seen this yet */
    "KNOWN UNKNOWN (16)",    /* I haven't seen this yet */
    "Frequency",
    "StartDate",
    "StopDate",
    /* Following two used when EventType is alarm */
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
    "ContactID"
};

alc_type in_counter = 0;
alc_type out_counter = 0;

alc_type recv_buffer[1024];
int recv_buffer_pos = 0;

void alcatel_send_packet(alc_type type, alc_type *data, alc_type len) {
    static alc_type buffer[1024];
    int size = 0, i, xor = 0;
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
        xor ^= buffer[i];
    buffer[size] = xor;
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
            if (fails > 100) {
                message(MSG_ERROR,"Reading failed...");
                return false;
//                exit(10);
            }
        }
    }
    return true;
}

void alcatel_recv_shorten(int size) {
    recv_buffer_pos -= size;
    memmove(recv_buffer, recv_buffer + size, recv_buffer_pos);
}

alc_type *alcatel_recv_packet(int ack) {
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
            message(MSG_DEBUG,"Disconnect ack - throwing away");
            wanted_size = 3;
            break;
        case ALC_CONNECT_ACK:
            message(MSG_DEBUG,"Connect ack - throwing away");
            wanted_size = 6;
            break;
        case ALC_CONTROL_ACK:
            message(MSG_DEBUG,"Control ack - don't know what it means - throwing away");
            wanted_size = 4;
            /* this is some magic ;-) */
            if (type == ALC_ACK) once_again = 1;
            break;
        case ALC_ACK:
            message(MSG_DEBUG,"Normal ack - throwing away");
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

void alcatel_init(){
    char answer[500];

    message(MSG_DETAIL,"Entering Alcatel binary mode");
    modem_cmd("AT+IFC=2,2\r",answer,sizeof(answer),100,0);
    modem_cmd("AT+CPROT=16,\"V1.0\",16\r",answer,sizeof(answer),100,"CONNECT");
    message(MSG_DEBUG,"Alcatel binary mode started");
    alcatel_send_packet(ALC_CONNECT,0,0);
    free(alcatel_recv_ack(ALC_CONNECT_ACK));
    usleep(SLEEP_CHANGE);
}

void alcatel_attach(){
    alc_type buffer[] = {0x00,0x00,0x7C,0x20};
    message(MSG_DETAIL, "Attaching to Alcatel");
    alcatel_send_packet(ALC_DATA, buffer, 4); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    message(MSG_DEBUG2,"Attached to Alcatel");
}

void alcatel_detach(){
    alc_type buffer[] = {0x00,0x01,0x7C,0x00};
    message(MSG_DETAIL, "Detaching from Alcatel");
    alcatel_send_packet(ALC_DATA, buffer, 4); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    message(MSG_DEBUG2, "Detached from Alcatel");
}

void alcatel_done(){
    message(MSG_DETAIL,"Leaving Alcatel binary mode");
    alcatel_send_packet(ALC_DISCONNECT,0,0);
    free(alcatel_recv_ack(ALC_DISCONNECT_ACK));
    usleep(SLEEP_CHANGE);
}

void sync_start_session() {
    alc_type buffer[] = {0x00, 0x04, 0x7C, 0x80, 0x12, 0x34, 0x56, 0x78};
//                                               ^^^^^^^^^^^^^^^^^^^^^^ - this is DBID but it was always same...
    
    message(MSG_INFO,"Starting sync session");
    alcatel_send_packet(ALC_DATA, buffer, 8); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
}

void sync_close_session(alc_type type) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x23, 0x01};
//                                                      ^^^^
// this is session id, but this software currently supports only one session...
    
    message(MSG_INFO,"Closing sync session");
    alcatel_send_packet(ALC_DATA, buffer, 5); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
}

int sync_select_type(alc_type type) {
    alc_type buffer[] = {0x00, 0x00, type | 0x60, 0x20};
    alc_type buffer2[] = {0x00, 0x04, type | 0x60, 0x22, 0x01, 0x00};

    int result;
    alc_type *answer;
    
    message(MSG_INFO,"Setting sync type: %02X", type);

    alcatel_send_packet(ALC_DATA, buffer, 4); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    
    alcatel_send_packet(ALC_DATA, buffer2, 6); 
    free(alcatel_recv_ack(ALC_ACK));
//    free(alcatel_recv_packet(1));

    answer = alcatel_recv_packet(1);
    if (!answer) return -1;

    result = answer[8];
    
    free(answer);

    if (result == 0)
        free(alcatel_recv_packet(1));

    return result;
}

void sync_begin_read(alc_type type) {
    alc_type buffer[] = {0x00, 0x04, 0x7C, 0x81, type, 0x00, 0x85, 0x00};
    
    alcatel_send_packet(ALC_DATA, buffer, 8); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    free(alcatel_recv_packet(1));
}

int *sync_get_ids(alc_type type) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x2F, 0x01};
    alc_type *data;
    int *result;
    int count = 0, i, pos;
    
    alcatel_send_packet(ALC_DATA, buffer, 5); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (!data) return NULL;

    count = data[10];

    result = (int *)malloc((count + 1)* sizeof(int));
    chk(result);

    result[0] = count;

    for (i = 0; i < count; i++) {
        pos = 11 + (4 * i);
        result[i + 1] = data[pos + 3] + (data[pos + 2] << 8) + (data[pos + 1] << 16) + (data[pos] << 24);
    }

    free(data);

    return result;
}

int *sync_get_fields(alc_type type, int item) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x30, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff)};
    alc_type *data;
    int *result;
    int count = 0, i;
    
    alcatel_send_packet(ALC_DATA, buffer, 9); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (!data) return NULL;

    count = data[14];

    result = (int *)malloc((count + 1)* sizeof(int));
    chk(result);

    result[0] = count;

    for (i = 0; i < count; i++) {
        result[i + 1] = data[15 + i];
    }

    return result;
}

alc_type *sync_get_field_value(alc_type type, int item, int field) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x1f, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff), (field & 0xff)};
    alc_type *data;
    alc_type *result;
    int len;

    alcatel_send_packet(ALC_DATA, buffer, 10); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    if (!data) return NULL;

    len = data[4] - 10; 

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
        message(MSG_DEBUG,"Decoded string: %s", s);
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

int *sync_get_obj_list(alc_type type, alc_type list) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x0b, list | 0x90};
    alc_type *data;
    int *result;
    int count = 0, i;
    
    alcatel_send_packet(ALC_DATA, buffer, 5); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);

    if (data[4] < 8) count = 0;
    else count = data[12];

    result = (int *)malloc((count + 1)* sizeof(int));
    chk(result);

    result[0] = count;

    for (i = 0; i < count; i++) {
        result[i + 1] = data[13 + i];
    }

    return result;
}

char *sync_get_obj_list_item(alc_type type, alc_type list, int item) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x0c, list | 0x90, 0x0A, 0x01, (item & 0xff) };
    alc_type *data;
    char *result;
    int len;
    int j;

    alcatel_send_packet(ALC_DATA, buffer, 8); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);

    len = data[14]; 

    result = (char *)malloc(len + 1);
    chk(result);

    memcpy(result,data + 15, len);

    for (j=0; j<=len; j++) result[j] = gsm2ascii(result[j]);

    free(data);
    
    result[len] = 0;
    
    return result;
}

void sync_del_obj_list_items(alc_type type, alc_type list) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x0e, list | 0x90};

    alcatel_send_packet(ALC_DATA, buffer, 5); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    free(alcatel_recv_packet(1));
}

int sync_create_obj_list_item(alc_type type, alc_type list, char *item) {
    alc_type buffer[256] = {0x00, 0x04, type | 0x60, 0x0d, list | 0x90, 0x0b };
    alc_type *data;
    int i;

    i = strlen(item);
    buffer[6] = (alc_type)(i + 1);
    buffer[7] = (alc_type)i;
    memcpy(buffer + 8, item, i);

    alcatel_send_packet(ALC_DATA, buffer, 8 + i); 
    free(alcatel_recv_ack(ALC_ACK));
    free(alcatel_recv_packet(1));
    data = alcatel_recv_packet(1);
    
    i = data[12]; 
    free(data);
    return i;
}

void sync_commit(alc_type type) {
    alc_type buffer[] = {0x00, 0x04, type | 0x60, 0x20, 0x01 };
    alc_type *data;
//                                                      ^^^^
// this is session id, but this software currently supports only one session...

    alcatel_send_packet(ALC_DATA, buffer, 5); 
    free(alcatel_recv_ack(ALC_ACK));
    data = alcatel_recv_packet(1);
    if (data[8] == 0) {
        free(alcatel_recv_packet(1));
//      7E 02 15 00 09 00 07 68 20 00 00 00 37 00 18
//                                 ^^^^^^^^^^^ should be returned....
    }
    free(data);
}

int sync_update_field(alc_type type, int item, int field, AlcatelFieldStruct *data) {
    alc_type buffer[180] = {0x00, 0x04, type | 0x60, 0x26, 0x01, (item >> 24), ((item >> 16) & 0xff), ((item >> 8) & 0xff), (item & 0xff), 
        0x65, 0x00 /* length of remaining part */, (field & 0xff), 0x37 /* here follows data */};
    alc_type *answer;
    int result;
    int j;
    
    switch (data->type) {
        case _date:
            buffer[10] = 0x09;
            buffer[13] = 0x05;
            buffer[14] = 0x67;
            buffer[15] = 0x04;
            buffer[16] = ((AlcatelDateStruct *)(data->data))->month;
            buffer[17] = ((AlcatelDateStruct *)(data->data))->day;
            buffer[18] = ((AlcatelDateStruct *)(data->data))->year >> 8;
            buffer[19] = ((AlcatelDateStruct *)(data->data))->year & 0xff;
            buffer[20] = 0x00;
            break;
        case _time:
            buffer[10] = 0x08;
            buffer[13] = 0x06;
            buffer[14] = 0x68;
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
            buffer[10] = 0x05;
            buffer[13] = 0x04;
            buffer[14] = 0x38;
            buffer[15] = *(int *)(data->data) & 0xff;
            buffer[16] = 0x00;
            break;
        case _bool:
            buffer[10] = 0x05;
            buffer[13] = 0x03;
            buffer[14] = 0x3b;
            buffer[15] = *(int *)(data->data) & 0xff;
            buffer[16] = 0x00;
            break;
        case _int:
            buffer[10] = 0x08;
            buffer[13] = 0x02;
            buffer[14] = 0x3a;
            buffer[15] = *(int *)(data->data) >> 24;
            buffer[16] = (*(int *)(data->data) >> 16) & 0xff;
            buffer[17] = (*(int *)(data->data) >> 8) & 0xff;
            buffer[18] = *(int *)(data->data) & 0xff;
            buffer[19] = 0x00;
            break;
        case _byte:
            buffer[10] = 0x05;
            buffer[13] = 0x00;
            buffer[14] = 0x38;
            buffer[15] = *(int *)(data->data) & 0xff;
            buffer[16] = 0x00;
            break;
    }

    alcatel_send_packet(ALC_DATA, buffer, 12 + buffer[10]); 
    free(alcatel_recv_ack(ALC_ACK));
    answer = alcatel_recv_packet(1);

    result = answer[8];

    free(answer);

    return result;
}

int sync_create_field(alc_type type, int field, AlcatelFieldStruct *data) {
    alc_type buffer[180] = {0x00, 0x04, type | 0x60, 0x25, 0x01, 0x65, 0x00 /* length of remaining part */, (field & 0xff), 0x37 /* here follows data */};
    alc_type *answer;
    int result;
    
    switch (data->type) {
        case _date:
            buffer[6] = 0x09;
            buffer[9] = 0x05;
            buffer[10] = 0x67;
            buffer[11] = 0x04;
            buffer[12] = ((AlcatelDateStruct *)(data->data))->month;
            buffer[13] = ((AlcatelDateStruct *)(data->data))->day;
            buffer[14] = ((AlcatelDateStruct *)(data->data))->year >> 8;
            buffer[15] = ((AlcatelDateStruct *)(data->data))->year & 0xff;
            buffer[16] = 0x00;
            break;
        case _time:
            buffer[6] = 0x08;
            buffer[9] = 0x06;
            buffer[10] = 0x68;
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
            buffer[6] = 0x05;
            buffer[9] = 0x04;
            buffer[10] = 0x38;
            buffer[11] = *(int *)(data->data) & 0xff;
            buffer[12] = 0x00;
            break;
        case _bool:
            buffer[6] = 0x05;
            buffer[9] = 0x03;
            buffer[10] = 0x3b;
            buffer[11] = *(int *)(data->data) & 0xff;
            buffer[12] = 0x00;
            break;
        case _int:
            buffer[6] = 0x08;
            buffer[9] = 0x02;
            buffer[10] = 0x3a;
            buffer[11] = *(int *)(data->data) >> 24;
            buffer[12] = (*(int *)(data->data) >> 16) & 0xff;
            buffer[13] = (*(int *)(data->data) >> 8) & 0xff;
            buffer[14] = *(int *)(data->data) & 0xff;
            buffer[15] = 0x00;
            break;
        case _byte:
            buffer[6] = 0x05;
            buffer[9] = 0x00;
            buffer[10] = 0x38;
            buffer[11] = *(int *)(data->data) & 0xff;
            buffer[12] = 0x00;
            break;
    }

    alcatel_send_packet(ALC_DATA, buffer, 8 + buffer[6]); 
    free(alcatel_recv_ack(ALC_ACK));
    answer = alcatel_recv_packet(1);

    result = answer[8];

    free(answer);

    return result;
}
