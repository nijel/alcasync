/*
 * alcatool/common.cpp
 *
 * some common functions
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "logging.h"

int is_number(const char* const text) {
    int i;
    int Length;
    Length=strlen(text);
    for (i=0; i<Length; i++)
        if (((text[i]>'9') || (text[i]<'0')) && (text[i]!='-'))
            return 0;
    return 1;
}

void chk(const void *p) {
    if (p) return;
    message(MSG_ERROR,"Virtual memory exhausted");
    exit(100);
}
