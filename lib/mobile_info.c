/* $Id$ */
#include <stdio.h>
#include <string.h>

#include "mobile_info.h"
#include "modem.h"
#include "logging.h"
#include "version.h"

char mobil_signal_info[][10] = {
    "<-113 dBm", 
    "-111 dBm", 
    "-109 dBm",
    "-107 dBm",
    "-105 dBm",
    "-103 dBm",
    "-101 dBm",
    "-99 dBm",
    "-97 dBm",
    "-95 dBm",
    "-93 dBm",
    "-91 dBm",
    "-89 dBm",
    "-87 dBm",
    "-85 dBm",
    "-83 dBm",
    "-81 dBm",
    "-79 dBm",
    "-77 dBm",
    "-75 dBm",
    "-73 dBm",
    "-71 dBm",
    "-69 dBm",
    "-67 dBm",
    "-65 dBm",
    "-63 dBm",
    "-61 dBm",
    "-59 dBm",
    "-57 dBm",
    "-55 dBm",
    "-53 dBm",
    ">-51 dBm"
};

int get_battery(int *mode, int *charge){
    char buffer[1000], *pos;
    modem_cmd("AT+CBC\r\n",buffer,sizeof(buffer)-1,50,0);
    
    pos = strstr(buffer, "+CBC:");
    if (pos == NULL) {
        message(MSG_WARNING,"Could not read battery status! (received: \"%s\")",reform(buffer,1));
        *mode = -1;
        *charge = -1;
    } else {
        sscanf(pos,"+CBC: %d, %d",mode,charge);
    }
    return *charge;
}

int get_signal(int *rssi, int *ber){
    char buffer[1000], *pos;
    modem_cmd("AT+CSQ\r\n",buffer,sizeof(buffer)-1,50,0);

    pos = strstr(buffer, "+CSQ:");
    if (pos == NULL) {
        message(MSG_WARNING,"Could not read signal quality! (received: \"%s\")",reform(buffer,1));
        *rssi = -1;
        *ber = -1;
    } else {
        sscanf(pos,"+CSQ: %d, %d",rssi,ber);
    }

    return *rssi;
}

void get_sn(char *sn,int len){
    get_string("AT+CGSN\r\n",sn,len);
}

void get_revision(char *rev,int len){
    get_string("AT+CGMR\r\n",rev,len);
}

void get_model(char *model,int len){
    get_string("AT+CGMM\r\n",model,len);
}

void get_manufacturer(char *manuf,int len){
    get_string("AT+CGMI\r\n",manuf,len);
}

void get_string(char *cmd, char *data, int len){
    char buffer[1000];
    char *loc1,*loc2;
    modem_cmd(cmd,buffer,sizeof(buffer)-1,50,0);
    loc1 = strchr(buffer,'\n');
    if (loc1 != NULL) {
        loc1++;
        loc2 = strchr(loc1,'\r');
        loc2[0] = '\0';
        strncpy(data,loc1,len-1);
    } else {
        message(MSG_WARNING,"Could not parse string from output! (received: \"%s\")",reform(buffer,1));
        data[0]='\0';
    }
    message(MSG_DEBUG,"Parsed string (len=%d): %s", len, reform(data,1));
}
