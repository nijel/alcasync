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

int  msg_level;

void message(int severity,char* format, ...);

const char *reform(const char *s,int slot);
const char *hexdump(const unsigned char *s, int size,int slot);

#endif
