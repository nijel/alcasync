/*
 * alcatool/mobile_info.h
 *
 * acquire information about mobile
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
#ifndef MODEM_INFO_H
#define MODEM_INFO_H

int get_battery(int *mode, int *change);
int get_signal(int *rssi, int *ber);
void get_manufacturer(char *manuf,int len);
void get_string(char *cmd, char *data, int len);
void get_sn(char *sn,int len);
void get_revision(char *rev,int len);
void get_model(char *model,int len);

extern char mobil_signal_info[][10];

#endif
