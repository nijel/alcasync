/*
 * alcasync/mobile_info.h
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
 * if and only if programmer/distributor of that code receives written
 * permission from author of this code.
 *
 */
/* $Id$ */
#ifndef MODEM_INFO_H
#define MODEM_INFO_H

/** Reads battery information
 */
int get_battery(int *mode, int *change);

/** Reads signal information
 */
int get_signal(int *rssi, int *ber);

/** Reads manufacturer
 */
void get_manufacturer(char *manuf,int len);

/** Reads serial number
 */
void get_sn(char *sn,int len);

/** Reads revision
 */
void get_revision(char *rev,int len);

/** Reads model information
 */
void get_model(char *model,int len);

/** Reads string value using AT command.
 * Used by
 * @ref get_manufacturer,
 * @ref get_sn,
 * @ref get_revision,
 * @ref get_model
 */
void get_string(const char *cmd, char *data, int len);

/** Reads IMSI (Internation Mobile Subscriber Identity)
 */
void get_imsi(char *manuf,int len);

/** Descriptions to signal strength values returned bu @ref get_signal
 */
extern char mobil_signal_info[][10];

#endif
