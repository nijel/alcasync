/* $Id$ */
#ifndef COMMON_H
#define COMMON_H

#undef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))

int is_number(const char* const text);

void chk(const void *p);

#endif
