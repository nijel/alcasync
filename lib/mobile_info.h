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
