/* $Id$ */
#define _GNU_SOURCE
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "pdu.h"
#include "common.h"
#include "mobile.h"
#include "charset.h"

int str2pdu(char *str, char *pdu, int charset_conv) {
    char numb[500];
    char octett[10];
    int pdubitposition;
    int pdubyteposition;
    int strLength;
    int character;
    int bit;
    int pdubitnr;
    char converted;
    strLength=strlen(str);
    for (character=0;character<sizeof(numb);character++)
        numb[character]=0;
    for (character=0;character<strLength;character++) {
        if (charset_conv)
            converted=ascii2gsm(str[character]);
        else
            converted=str[character];
        for (bit=0;bit<7;bit++) {
            pdubitnr=7*character+bit;
            pdubyteposition=pdubitnr/8;
            pdubitposition=pdubitnr%8;
            if (converted & (1<<bit))
                numb[pdubyteposition]=numb[pdubyteposition] | (1<<pdubitposition);
            else
                numb[pdubyteposition]=numb[pdubyteposition] & ~(1<<pdubitposition);
        }
    }
    numb[pdubyteposition+1]=0;
    pdu[0]=0;
    for (character=0;character<=pdubyteposition; character++) {
        sprintf(octett,"%02X",(unsigned char) numb[character]);
        strcat(pdu,octett);
    }
    return pdubyteposition;
}

int octet2bin(char* octet) {
    int result=0;
    if (octet[0]>57)
        result+=octet[0]-55;
    else
        result+=octet[0]-48;
    result=result<<4;
    if (octet[1]>57)
        result+=octet[1]-55;
    else
        result+=octet[1]-48;
    return result;
}

int pdu2str(char *pdu, char *str, int charset_conv) {
    int bitposition=0;              
    int byteposition;
    int byteoffset;
    int charcounter;
    int bitcounter;
    int count;
    int octetcounter;
    char c;
    char binary[500];

    /* First convert all octets to bytes */
    count=octet2bin(pdu);
    for (octetcounter=0; octetcounter<count; octetcounter++)
        binary[octetcounter]=octet2bin(pdu+(octetcounter<<1)+2);

    /* Then convert from 8-Bit to 7-Bit encapsulated in 8 bit */
    for (charcounter=0; charcounter<count; charcounter++) {
        c=0;
        for (bitcounter=0; bitcounter<7; bitcounter++) {
            byteposition=bitposition/8;
            byteoffset=bitposition%8;
            if (binary[byteposition]&(1<<byteoffset))
                c=c|128;
            bitposition++;
            c=(c>>1)&127; /* The shift fills with 1, but I want 0 */
        }
        if (charset_conv)
            str[charcounter]=gsm2ascii(c);
        else if (c==0)
            str[charcounter]=183;
        else
            str[charcounter]=c;
    }
    str[count]=0;
    return count;
}

void swapchars(char* string) {
    int Length;
    int position;
    char c;
    Length=strlen(string);
    for (position=0; position<Length-1; position+=2) {
        c=string[position];
        string[position]=string[position+1];
        string[position+1]=c;
    }
}

int split_pdu(char *pdu, char *sendr, time_t *date, char *ascii, char *smsc) {
    int Length;
    int padding;
    int type;
    char *Pointer;
    char numb[]="00";
    struct tm time;

    sendr[0]=0;
    ascii[0]=0;
    smsc[0]=0;
    // get senders smsc
    Length=octet2bin(pdu)*2-2;
    if (Length>0)
    {
        type = octet2bin(pdu + 2);
        Pointer=pdu+4;
        strncpy(smsc,Pointer,Length);
        swapchars(smsc);
        /* remove Padding characters after swapping */
        if (smsc[Length-1]=='F')
            smsc[Length-1]=0;
        else
            smsc[Length]=0;
        if (type == NUM_INT) {
            memmove(smsc + 1, smsc, strlen(smsc));
            smsc[0] = '+';
        }
    }
    Pointer=pdu+Length+4;
    if (octet2bin(Pointer) == 0x11) {
        Pointer += 4;
        Length=octet2bin(Pointer);
        padding=Length%2;
        type = octet2bin(Pointer + 2);
        Pointer += 4;
        strncpy(sendr,Pointer,Length+padding);
        swapchars(sendr);
        sendr[Length]=0;
        if (type == NUM_INT) {
            memmove(sendr + 1, sendr, strlen(sendr));
            sendr[0] = '+';
        }
        Pointer += Length + padding + 6;
        *date = -1;
    } else {
        Pointer += 2;
        Length=octet2bin(Pointer);
        padding=Length%2;
        type = octet2bin(Pointer + 2);
        Pointer+=4;
        strncpy(sendr,Pointer,Length+padding);
        swapchars(sendr);
        sendr[Length]=0;
        if (type == NUM_INT) {
            memmove(sendr + 1, sendr, strlen(sendr));
            sendr[0] = '+';
        }
        Pointer=Pointer+Length+padding+4; // skip protocol and coding scheme
        /* extract date */
        numb[0]=Pointer[5];
        numb[1]=Pointer[4];
        time.tm_mday = atoi(numb);
        numb[0]=Pointer[3];
        numb[1]=Pointer[2];
        time.tm_mon = atoi(numb)-1;
        numb[0]=Pointer[1];
        numb[1]=Pointer[0];
        time.tm_year = 100 + atoi(numb);
        Pointer=Pointer+6;
        numb[0]=Pointer[5];
        numb[1]=Pointer[4];
        time.tm_sec = atoi(numb);
        numb[0]=Pointer[3];
        numb[1]=Pointer[2];
        time.tm_min = atoi(numb);
        numb[0]=Pointer[1];
        numb[1]=Pointer[0];
        time.tm_hour = atoi(numb);
        *date = mktime(&time);

        Pointer=Pointer+8; //skip time zone
    }
    /* read message content */
    return pdu2str(Pointer,ascii,1);
}


char *make_pdu_number(char *number, int add) {
    char *result;
    char tmp[100];
    int num_type, num_len;

    num_len = strlen(number);

    if (number[0] == '+') {
        strncpy(tmp, number + 1, PDU_MAXNUMLEN);
        num_type = NUM_INT;
        num_len--;
    } else {
        strncpy(tmp, number, PDU_MAXNUMLEN);
        num_type = NUM_NAT;
    }

    /* terminate the number with F if the length is odd */
    if (strlen(tmp) % 2)
        strcat(tmp, "F");

    /* Swap every second character */
    swapchars(tmp);
    
    chk(result = malloc(strlen(tmp) + 3));

    sprintf(result, "%02X%02X%s", (add != 0)?((num_len/2)+1):num_len, num_type, tmp);
    
    return result;
}


void make_pdu_smsc(char *smsc, char* number, char* message, int deliv_report, int pdu_class, char* pdu) {
    /* this currently doesn't work as expected ... */
    char pdu_msg[500];
    char *pdu_number, *pdu_smsc;
    char *msg;
    char stime[13];
    struct tm *stamp;
    time_t t;
    
    t = time(NULL);
    stamp = localtime(&t);

    pdu_smsc = make_pdu_number(smsc, 2);
    pdu_number = make_pdu_number(number, 0);
    
    msg = strndup(message,PDU_MAXBODYLEN);

    str2pdu(msg, pdu_msg, 1);

    sprintf(stime, "%02d%02d%02d%02d%02d%02d", 
            stamp->tm_year - 100,
            stamp->tm_mon + 1,
            stamp->tm_mday, 
            stamp->tm_hour,
            stamp->tm_min,
            stamp->tm_sec);
    
    swapchars(stime);

    sprintf(pdu, "%s00%s0000%s00%02X%s", pdu_smsc, pdu_number, stime, strlen(msg), pdu_msg);
}

void make_pdu(char* number, char* message, int deliv_report, int pdu_class, char* pdu) {
    char pdu_msg[500];
    char *pdu_number;
    char *msg;
    int pdu_type, pdu_coding_scheme;

    pdu_number = make_pdu_number(number, 0);
    
    msg = strndup(message,PDU_MAXBODYLEN);

    pdu_type = 16 + 1; /* Validity Field + SMS-Submit MS to SMSC */
				/* 64 = user data */
    pdu_coding_scheme = 240 + 0 + pdu_class; /* Dummy + 7 Bit + Class */
						/* 4 = 8 Bit */
	if (deliv_report)
		pdu_type = pdu_type + 32; /* Request Status Report */

    str2pdu(msg, pdu_msg, 1);

    sprintf(pdu, "00%02X00%s00%02XFF%02X%s", pdu_type, pdu_number, pdu_coding_scheme, strlen(msg), pdu_msg);
//                                ^^ validity
//                          ^^ protocol identifier
//                      ^^ message reference
}

