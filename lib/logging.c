/***************************************************************************
                          logging.c  -  description
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
/* $Id$ */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <assert.h>
#include <limits.h>
#include <ctype.h>

#include "version.h"
#include "common.h"
#include "logging.h"


char msg_level_info[][8] = {
        "DEBUG2",
        "DEBUG",
        "DETAIL",
        "INFO",
        "WARNING",
        "ERROR"
};

void message(int severity,char* format, ...)
{
    va_list argp;
    char text[1024];
    va_start(argp,format);
    vsnprintf(text,sizeof(text),format,argp);
    va_end(argp);
    if (severity>=msg_level) {
        if (msg_level > MSG_INFO || severity != MSG_INFO) {
            fprintf(stderr,"%s: %s\n", msg_level_info[severity], text);
        } else {
            fprintf(stderr,"%s\n", text);
        } 
    }
}

#define NELEM(x) (sizeof((x))/sizeof(*(x)))


const char *reform(const char *s,int slot) {
    static struct formslot {
        char *s;
        size_t l;
    } arr[3];
    char c,*d;
    struct formslot *fs;

    assert(slot>=0 && slot<NELEM(arr));
    if (!s) return("<unset>");
    if (!(fs=&arr[slot])->s)
        chk(fs->s=malloc(fs->l=LINE_MAX));
    d=fs->s;
    for (*d++='"';(c=*s);s++) {
        if (d>=fs->s+fs->l-10) {
            off_t o=d-fs->s;
            chk(fs->s=realloc(fs->s,(fs->l=(fs->l?fs->l*2:LINE_MAX))));
            d=fs->s+o;
        }
        if (c!='\\' && c!='"' && isprint(c)) { *d++=c; continue; }
        *d++='\\';
        switch (c) {
            case '\\': case '"': *d++=c; break;
            case '\n': *d++='n'; break;
            case '\r': *d++='r'; break;
            case '\032': *d++='Z'; break;
            case '\033': *d++='e'; break;
            default:
                d+=sprintf(d,"x%02X",(unsigned char)c);
                break;
        }
    }
    *d++='"'; *d='\0';
    return(fs->s);
}

const char *hexdump(const char *s, int size,int slot) {
    static struct formslot {
        char *s;
        size_t l;
    } arr[3];
    char c,*d;
    int i=0;
    struct formslot *fs;

    assert(slot>=0 && slot<NELEM(arr));
    if (!s) return("<unset>");
    if (!(fs=&arr[slot])->s)
        chk(fs->s=malloc(fs->l=LINE_MAX));
    d=fs->s;
    for (*d++='"';i<size;i++,s++) {
        c=*s;
        if (d>=fs->s+fs->l-10) {
            off_t o=d-fs->s;
            chk(fs->s=realloc(fs->s,(fs->l=(fs->l?fs->l*2:LINE_MAX))));
            d=fs->s+o;
        }
        d+=sprintf(d,"%02X ",(unsigned char)c);
    }
    *d++='"'; *d='\0';
    return(fs->s);
}
