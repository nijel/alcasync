#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "pdu.h"
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

int splitpdu(char *pdu, char *sendr, time_t *date, char *ascii, char *smsc) {
    int Length;
    int padding;
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
        Pointer=pdu+4;
        strncpy(smsc,Pointer,Length);
        swapchars(smsc);
        /* remove Padding characters after swapping */
        if (smsc[Length-1]=='F')
            smsc[Length-1]=0;
        else
            smsc[Length]=0;
    }
    Pointer=pdu+Length+6;  
    Length=octet2bin(Pointer);
    padding=Length%2;
    Pointer+=4;
    strncpy(sendr,Pointer,Length+padding);
    swapchars(sendr);
    /* remove Padding characters after swapping */
    sendr[Length]=0;
    Pointer=Pointer+Length+padding+3;
    /* extract date */
    Pointer++;
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

    Pointer=Pointer+8;
    
    /* read message content */
    return pdu2str(Pointer,ascii,1);
} 

/* make the PDU string. The destination variable pdu has to be big enough. */
void make_pdu(char* nummer, char* message, int messagelen, int report, char* pdu)
{
  int coding;
  int flags;
  char tmp[500];
  strcpy(tmp,nummer);
  /* terminate the number with F if the length is odd */
  if (strlen(tmp)%2)
    strcat(tmp,"F");
  /* Swap every second character */
  swapchars(tmp);
    flags=1; /* SMS-Submit MS to SMSC */
    coding=240+0+1; /* Dummy + 7 Bit + Class 1 */
  if (report)
    flags=flags+32; /* Request Status Report */
  /* concatenate the first part of the PDU string */
    sprintf(pdu,"00%02X00%02X91%s00%02XA7%02X",flags,strlen(nummer),tmp,coding,messagelen);
  /* Create the PDU string of the message */
    str2pdu(message,tmp,1);
  /* concatenate the text to the PDU string */
  strcat(pdu,tmp);
}

