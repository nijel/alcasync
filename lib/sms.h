/* $Id$ */
#ifndef SMS_H
#define SMS_H
#include <time.h>

#define SMS_UNREAD  0
#define SMS_READ    1
#define SMS_UNSENT  2
#define SMS_SENT    3
#define SMS_ALL     4

typedef struct {
    int pos;
    int stat;
    int len;
    char* raw;
    char* sendr;
    time_t date;
    char* ascii;
    char* smsc;
} SMS;


int delete_sms(int which);

SMS *get_sms(int which);
SMS *get_smss(int state);

int send_sms(char *pdu);
int put_sms(char *pdu, int state);

char *get_smsc(void);

#endif
