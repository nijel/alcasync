#ifndef SMS_H
#define SMS_H
#include <time.h>

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
SMS *get_smss();

#endif
