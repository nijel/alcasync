/*
 * alcatool/common.h
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
 * if and only if programmer/distributor of that code receives written
 * permission from author of this code.
 *
 */
/* $Id$ */
#ifndef COMMON_H
#define COMMON_H

#undef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))

/** Returns true id char* is number
 */
int is_number(const char* const text);

/** Terminates program in p is null, used for checking after malloc
 */
void chk(const void *p);

#endif
