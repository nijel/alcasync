/*
 * alcatool/logging.h
 *
 * loging functions with configurable verbosity
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
#ifndef LOGGING_H
#define LOGGING_H

#define MSG_ALL     0
#define MSG_NORMAL  3
#define MSG_NONE    6

#define MSG_DEBUG2  0
#define MSG_DEBUG   1
#define MSG_DETAIL  2
#define MSG_INFO    3
#define MSG_WARNING 4
#define MSG_ERROR   5

extern int  msg_level;

void message(int severity,char* format, ...);

const char *reform(const char *s,int slot);
const char *hexdump(const unsigned char *s, int size,int slot);

#endif
