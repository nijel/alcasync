/*****************************************************************************
 * alcatel.h - low level functions for communication with  Alcatel One Touch *
 *             501 and compatible mobile phone                               *
 *                                                                           *
 * Copyright (c) 2002 Michal Cihar <cihar at email dot cz>                   *
 *                                                                           *
 * This is just header file, for details see alcatel.c                       *
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

#ifndef ALCATEL_H
#define ALCATEL_H

/* packet types: */
/* used for starting binary connection (must be preceeded by 
 * AT+CPROT=16,"V1.0",16 and phone should respons to it by CONNECT) */
#define ALC_CONNECT         0x0A 
/* received when connect suceeded */
#define ALC_CONNECT_ACK     0x0C
/* used for stopping binary connection */
#define ALC_DISCONNECT      0x0D
/* received when binnary connection ends */
#define ALC_DISCONNECT_ACK  0x0E
/* some control ack, I really don't know what should it do, so currently it
 * is just ignored */
#define ALC_CONTROL_ACK     0x0F
/* sending/recieving data */
#define ALC_DATA            0x02
/* acknowledge to data */
#define ALC_ACK             0x06

/* synchronisation types (for sync_select_type, sync_get_ids, sync_get_fields): */
#define ALC_SYNC_TYPE_CALENDAR   0x04
#define ALC_SYNC_TYPE_TODO       0x08
#define ALC_SYNC_TYPE_CONTACTS   0x0C

/* synchronisation types (for sync_begin_read): */
#define ALC_SYNC_CALENDAR   0x00
#define ALC_SYNC_TODO       0x02
#define ALC_SYNC_CONTACTS   0x01

/* for reading list of categories (in todo and contacts) */
#define ALC_LIST_TODO_CAT       0x0B
#define ALC_LIST_CONTACTS_CAT   0x06

/* type used for I/O with mobile */
#define alc_type            unsigned char

/* types of return values */
typedef enum {
/*  name           stored as */    
    _date,      /* DATE      */
    _time,      /* TIME      */
    _string,    /* char *    */
    _phone,     /* char *    */
    _enum,      /* int       */
    _bool,      /* int       */
    _int,       /* int       */
    _byte       /* int       */
} TYPE;

typedef struct {
    TYPE type;
    void *data;
} FIELD;

typedef struct {
    int day;
    int month;
    int year;
} DATE;

typedef struct {
    int hour;
    int minute;
    int second;
} TIME;

/* initialises binary mode */
void alcatel_init();

/* ends binary mode */
void alcatel_done();

/* send packet of type type, if type = ACL_DATA then data are read from data
 * (length len, \0 is not treated as end) */
void alcatel_send_packet(alc_type type, alc_type *data, alc_type len);

/* attach to mobile, this must be used before any action */
void alcatel_attach();

/* detach from mobile, must be used before done */
void alcatel_detach();

/* start synchronisation session */
void sync_start_session();

void sync_close_session(alc_type type);

/* select synchronisation type */
void sync_select_type(alc_type type);

/* Start reading of selected type, do NOT use here ALC_SYNC_TYPE_* use the
 * ALC_SYNC_name instead. */
void sync_begin_read(alc_type type);

/* Returns array with ids of items of currently selected type. First item in
 * array contains length of it. */
int *sync_get_ids(alc_type type);

/* Returns array with field ids for selected item of currently selected type.
 * First item in array contains length of it. */
int *sync_get_fields(alc_type type, int item);

/* Returns array with data from selected field. First item in array contains
 * length of it. Following containg raw data as received from mobile, use 
 * decode_field_value to get them more readable. */
alc_type *sync_get_field_value(alc_type type, int item, int field);

/* Decodes raw field value to FIELD structure. */
FIELD *decode_field_value(alc_type *buffer);

/* Returns array with ids of categories. First item in array contains length
 * of it. */
int *sync_get_obj_list(alc_type type, alc_type list);

/* Returns name for selected category. */
char *sync_get_obj_list_item(alc_type type, alc_type list, int item);

#endif
