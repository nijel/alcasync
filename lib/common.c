/* $Id$ */
#include <stdio.h>

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
