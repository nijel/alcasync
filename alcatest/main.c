/*****************************************************************************
 * alcatest/main.c - testing program for alcatel communication               *
 *                                                                           *
 * parts of code (loading and parsing of commands) taken and adapted form    *
 * hddtemp by Emmanuel VARAGNAT <coredump@free.fr>                           *
 *                                                                           *
 * Copyright (c) 2002 Michal Cihar <cihar at email dot cz>                   *
 *                                                                           *
 * This is  EXPERIMANTAL  implementation  of comunication  protocol  used by *
 * Alcatel  501 (probably also any 50x and 70x) mobile phone. This  code may *
 * destroy your phone, so use it carefully. However whith my phone work this *
 * code correctly. This code assumes following conditions:                   *
 *  - no packet is lost                                                      *
 *  - 0x0F ack doesn't mean anything important                               *
 *  - data will be received as they are expected                             *
 *  - no error will appear in transmission                                   *
 *  - all magic numbers mean that, what I thing that they mean ;-)           *
 *                                                                           *
 * This program is  free software; you can  redistribute it and/or modify it *
 * under the terms of the  GNU  General  Public  License as published by the *
 * Free  Software  Foundation; either  version 2 of the License, or (at your *
 * option) any later version.                                                *
 *                                                                           *
 * This code is distributed in the hope that it will  be useful, but WITHOUT *
 * ANY  WARRANTY; without even the  implied  warranty of  MERCHANTABILITY or *
 * FITNESS FOR A  PARTICULAR PURPOSE. See the GNU General Public License for *
 * more details.                                                             *
 *                                                                           *
 * In addition to GNU GPL this code may be used also in non GPL programs but *
 * if and  only if  programmer/distributor  of that  code  receives  written *
 * permission from author of this code.                                      *
 *                                                                           *
 *****************************************************************************/
/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <libgen.h>
#include <string.h>

#include "modem.h"
#include "mobile_info.h"
#include "logging.h"
#include "version.h"
#include "charset.h"
#include "sms.h"
#include "common.h"
#include "pdu.h"
#include "alcatel.h"

#define MAX_LINE_LEN    2048
#define ALCATEST        "AlcaTest"

char device_name[] = "/dev/ttyS1";
char lock_name[] = "/var/lock/LCK..ttyS1";
char init[] = "AT S7=45 S0=0 L1 V1 X4 &c1 E1 Q0";

enum param_type {
    Tstring = 'S',
    Thex    = 'X',
    Tdword  = 'D',
    Tword   = 'W',
    Tbyte   = 'B'
};

struct param_data {
    char *description;
    param_type type;
    int min_length;
    int max_length;
    param_data *next;
};

struct command_data {
    char *name;
    char *description;
    char *data;
    int params;
    int reads;
    param_data *param_info;
    command_data *next;
};

command_data *commands = NULL;
command_data **last_command = &commands;


const char *extract_data(char **string) {
  char *str;

  if(**string != '"')
    return NULL;
  else
    (*string)++;

  str = *string;

  for(; *str; str++) {
    switch(*str) {
      case '"':
         return str;
         break;
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
         break;
      case '%':
         str++;
         switch (*str) {
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                break;
            default:
                printf("Bad %% argument!\n");
                return NULL;
         }
         break;
/*      case '\\':
         str++;
         break;*/
        default:
            printf("Bad char in data!\n");
            return NULL;
    }
  }

  return NULL;
}

const char *extract_string(char **string) {
  char *str;

  if(**string != '"')
    return NULL;
  else
    (*string)++;

  str = *string;

  for(; *str; str++) {
    switch(*str) {
      case '"':
        return str;
        break;
/*      case '\\':
         str++;
         break;*/
    }
  }

  return NULL;
}

bool parse_cmd_line(char *line) {
    const char      *name,
                    *description,
                    *data;
    command_data    *new_entry;
    int             value;
    int             read;
    char            *p;
    
    param_data      *params = NULL;
    param_data      **last_param = &params;

  line += strspn(line, " \t");
  if(*line == '#' || *line == '\0')
    return true;

  /* extract cmd name */
  p = (char*) extract_string(&line);
  if(p == NULL || *p == '\0')
    return false;

  *p = '\0';
  name = line;
  line = p + 1;


  /* extract description */
  line += strspn(line, " \t");
  if(! *line)
    return false;

  p = (char*) extract_string(&line);
  if(p == NULL)
    return false;

  description = line;
  *p = '\0';
  line = p + 1;
  
  
  /* extract data */
  line += strspn(line, " \t");
  if(! *line)
    return false;

  p = (char*) extract_data(&line);
  if(p == NULL)
    return false;

  data = line;
  *p = '\0';
  line = p + 1;
  
  
  /* extract read count */
  line += strspn(line, " \t");
  if(! *line)
    return false;

  p = line;
  read = 0;
  while(*p >= '0' && *p <= '9') {
    read = 10*read + *p-'0';
    p++;
  }
  if(read > 0 && (*p == '\0' || ( *p != ' ' && *p != '\t')) )
    return false;
  line = p;

  /* extract param count */
  line += strspn(line, " \t");
  if(! *line)
    return false;

  p = line;
  value = 0;
  while(*p >= '0' && *p <= '9') {
    value = 10*value + *p-'0';
    p++;
  }
  if(value > 0 && (*p == '\0' || ( *p != ' ' && *p != '\t')) )
    return false;
  line = p;
  
  /* extract parameters */

    if (value > 0) {
        for (int i=0; i<value; i++) {
            param_data      *new_param;
            param_type      type;
            char *pdesc;
            int min_length;
            int max_length;

            if(! *line)
                return false;
            line = strchr(line, '{');
            if(!*line)
                return false;
            line++;

            /* read type */
            if(!*line)
                return false;
            if (*line == 'S') {
                type = Tstring;
            } else if (*line == 'X') {
                type = Thex;
            } else if (*line == 'D') {
                type = Tdword;
            } else if (*line == 'W') {
                type = Tword;
            } else if (*line == 'B') {
                type = Tbyte;
            } else {
                return false;
            }
            line++;

            /* extract description */
            line += strspn(line, " \t");
            if(! *line)
                return false;

            p = (char*) extract_string(&line);
            if(p == NULL)
                return false;

            pdesc = line;
            *p = '\0';
            line = p + 1;
            
            min_length = 0;
            max_length = 0;

            if (type == Tstring || type == Thex) {
                /* extract min */
                line += strspn(line, " \t");
                if(! *line)
                    return false;

                p = line;
                while(*p >= '0' && *p <= '9') {
                    min_length = 10*min_length + *p-'0';
                    p++;
                }
                if(*p == '\0' || ( *p != ' ' && *p != '\t') )
                    return false;
                line = p;

                /* extract max */
                line += strspn(line, " \t");
                if(! *line)
                    return false;

                p = line;
                while(*p >= '0' && *p <= '9') {
                    max_length = 10*max_length + *p-'0';
                    p++;
                }
                if(*p == '\0' || ( *p != ' ' && *p != '\t' && *p != '}') )
                    return false;
                line = p;
            }
            line = strchr(line, '}');
            if(! *line)
                return false;
            line++;

            new_param = (param_data *) malloc(sizeof(param_data));
            if(new_param == NULL) {
                perror("malloc");
                exit(-1);
            }
            new_param->description = strdup(pdesc);
            new_param->type = type;
            new_param->min_length = min_length;
            new_param->max_length = max_length;
            new_param->next = NULL;
            *last_param = new_param;
            last_param = &(new_param->next);
        }
  }


  new_entry = (command_data *) malloc(sizeof(command_data));
  if(new_entry == NULL) {
    perror("malloc");
    exit(-1);
  }
  new_entry->name = strdup(name);
  new_entry->description = strdup(description);
  new_entry->data = strdup(data);
  new_entry->reads = read;
  new_entry->params = value;
  new_entry->param_info = params;
  new_entry->next = NULL;
  *last_command = new_entry;
  last_command = &(new_entry->next);

  return true;
}

void load_commands(char *cmd_file) {
    char  buff[MAX_LINE_LEN];
    char  *p, *s, *e, *ee;
    int   fd;
    int   n, rest, numline;
    
    fd = open(cmd_file, O_RDONLY);
    if (fd == -1) {
        printf ("Cannot open alcatest.cmd, terminating...\n");
        exit(1);
    }
    numline = 0;
    rest = MAX_LINE_LEN;
    p = buff;
    
    while ((n = read(fd, p, rest)) > 0) {
        s = buff;
        ee = p + n;
        
        while ((e = (char *)memchr(s, '\n', ee-s))) {
            *e = '\0';
            numline++;
            if (!parse_cmd_line(s)) {
                printf("ERROR: syntax error at line %d in %s\n", numline, cmd_file);
                exit(2);
            }
            s = e+1;
        }
        
        if (s == buff) {
            printf("ERROR: a line exceed %d characters in %s.\n", MAX_LINE_LEN, cmd_file);
            close(fd);
            exit(2);
        }

        memmove(buff, s, ee-s); /* because memory can overlap */

        rest = MAX_LINE_LEN - (ee-s);
        p = buff + (ee-s);
    }

    close(fd);
}

alc_type from_hex(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else {
        printf ("Bad hex char %c!\n", c);
        return 0;
    }
}

void show_string(char *buffer) {
    alc_type raw[1024];
    int i;
    int len = strlen(buffer);
    if (len % 2 != 0) {
        printf ("Bad data length!\n");
        return;
    }
    len >>= 1;
    for (i = 0; i < len; i++) 
        raw[i] =  (from_hex(buffer[2*i]) << 4)  + from_hex(buffer[2*i+1]);
    for (i=0; i<len; i++) raw[i] = gsm2ascii(raw[i]);
    raw[len] = '\0';
    printf("%s\n", raw);
}

void read_data(){
    alc_type *data;
    data = alcatel_recv_packet(true);
    if (data == NULL) {
        printf ("No data received!\n");
    } else {
        printf("Received data:\n");
        for (int i=0; i<data[4] + 6; i++){
            printf("%02X", data[i]);
            if (i % 2 == 0) printf (" ");
        }
        printf("\n");
        free(data);
    }
}

bool write_data(char *buffer){
    alc_type raw[1024];
    alc_type *data;
    
    int i;
    int len = strlen(buffer);
    if (len % 2 != 0) {
        printf ("Bad data length!\n");
        return false;
    }
    len >>= 1;
    for (i = 0; i < len; i++) 
        raw[i] =  (from_hex(buffer[2*i]) << 4)  + from_hex(buffer[2*i+1]);
    printf ("Sending packet: %s\n", buffer);
    alcatel_send_packet(ALC_DATA, raw, len);
    printf ("Receiving ack\n");
    data = alcatel_recv_ack(ALC_ACK);
    if (data == NULL) {
        printf ("Ack not received!\n");
        return false;
    } else {
        free(data);
        return true;
    }
}

bool do_command(char *command) {
    char buff[MAX_LINE_LEN];
    char *params[10];
    int pos, offset;
    command_data  *p;
    param_data  *par;

    for(p = commands; p; p = p->next) {
        if (strcmp(command, p->name) == 0) {
            printf("Executing %s (%s)\n"
                   ,p->name
                   ,p->description
                  );
            if (p->params > 0) {
                printf("Enter parameters:\n");
                int i = 0;
                for (par = p->param_info; par; par=par->next) {
                    char inp[1024];
                    int len;
                    unsigned int data;
                    switch (par->type) {
                        case Tstring:
                            do {
                                printf("Param%2d [%c] (%d-%d)\"%s\":"
                                       , i
                                       , par->type
                                       , par->min_length
                                       , par->max_length
                                       , par->description
                                      );
                                scanf("%s", inp);
                                len = strlen(inp);
                            } while (len < par->min_length || len > par->max_length);
                            params[i] = strdup(inp);
                            for( int j=0; params[i][j]; j++) 
                                params[i][j] = ascii2gsm(params[i][j]);
                            break;
                        case Thex:
                            do {
                                printf("Param%2d [%c] (%d-%d)\"%s\":"
                                       , i
                                       , par->type
                                       , par->min_length
                                       , par->max_length
                                       , par->description
                                      );
                                scanf("%s", inp);
                                len = strlen(inp);
                                for ( int j=0; inp[j]; j++) {
                                    switch (inp[j]) {
                                        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                                        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                                        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                                            break;
                                        default:
                                            len = -999;
                                            break;
                                    }
                                }
                            } while (len < par->min_length*2 || len > par->max_length*2);
                            params[i] = strdup(inp);
                            break;
                        case Tdword:
                            printf("Param%2d [%c] \"%s\":"
                                   , i
                                   , par->type
                                   , par->description
                                  );
                            scanf("%d", &data);
                            sprintf(inp, "%02X%02X%02X%02X", data >> 24, ((data >> 16) & 0xff), ((data >> 8) & 0xff), (data & 0xff));
                            params[i] = strdup(inp);
                            break;
                        case Tword:
                            printf("Param%2d [%c] \"%s\":"
                                   , i
                                   , par->type
                                   , par->description
                                  );
                            scanf("%d", &data);
                            sprintf(inp, "%02X%02X", ((data >> 8) & 0xff), (data & 0xff));
                            params[i] = strdup(inp);
                            break;
                        case Tbyte:
                            printf("Param%2d [%c] \"%s\":"
                                   , i
                                   , par->type
                                   , par->description
                                  );
                            scanf("%d", &data);
                            sprintf(inp, "%02X", (data & 0xff));
                            params[i] = strdup(inp);
                            break;
                    }

                    i++;
                }
            }
            for (pos = 0, offset = 0; p->data[pos]; pos++) {
                if (p->data[pos] == '%') {
                    pos ++;
                    if (p->data[pos] - '0' >= p->params) {
                        printf("Bad data string, contains %%%c\n", p->data[pos]);
                        return false;
                    }
                    strcpy(buff + pos + offset - 1, params[p->data[pos] - '0']);
                    offset += strlen(params[p->data[pos] - '0']) - 2;
                } else {
                    buff[pos + offset] = p->data[pos];
                }
            }
            buff[pos + offset] = '\0';
            write_data(buff);
            for (int i = 0; i<p->reads; i++) 
                read_data();
            return true;
        }
    }
    return false;
}

void show_command_info(char *command) {
    command_data  *p;
    param_data  *par;
    for(p = commands; p; p = p->next) {
        if (strcmp(command, p->name) == 0) {
            printf("Command:\t%s\n"
                   "Description:\t%s\n"
                   "Data:\t\t%s\n"
                   "Reads:\t\t%d\n"
                   "Parameters:\t%d\n"
                   ,p->name
                   ,p->description
                   ,p->data
                   ,p->reads
                   ,p->params
                  );
            if (p->params > 0) {
                printf("Id | Type | Min | Max | Description\n"
                       "-----------------------------------\n"
                       );
                int i = 0;
                for (par = p->param_info; par; par=par->next) {
                    printf("%2d |    %c | %3d | %3d | %s\n"
                           , i
                           , par->type
                           , par->min_length
                           , par->max_length
                           , par->description
                          );
                    i++;
                }
            }
            return;
        }
    }
    printf("External command %s not found!\n", command);
}

void show_loaded_commands() {
  char          *tabs, *line;
  command_data  *p;
  int           max, len;

  max = 0;
  for(p = commands; p; p = p->next) {
    len = strlen(p->name);
    if(len > max)
      max = len;
  }
  len = max/8 + 1;
  tabs = (char *)malloc(len+1);
  memset(tabs, '\t', len);
  tabs[len] = '\0';

  len = (max/8 + 1) * 8;
  line = (char *)malloc(len+1);
  memset(line, '-', len);
  line[len] = '\0';

  printf("\n"
     "Name%s| Params | Reads |Description\n"
     "----%s-----------------------------\n", tabs, line);

  for(p = commands; p; p = p->next) {
    len = strlen(p->name);
    printf("%s%s| %6d | %5d | %s\n",
       p->name,
       tabs+(len/8),
       p->params,
       p->reads,
       p->description);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
    char data[1024];
//    char *s;
    char command[1024];
    int len;
//    int i, j;

    rate = 19200;
    baudrate =  B19200;
    strcpy(device, device_name);
    strcpy(lockname, lock_name);
    strcpy(initstring, init);
    
    msg_level = MSG_DEBUG;

    printf(ALCATEST " - test program for alcatel communication\n");
    printf("Compiled with " ALCASYNC_NAME " version " ALCASYNC_VERSION "\n");
    printf("Copyright (c) " ALCASYNC_COPYRIGHT "\n\n");
    printf("This program uses very dummy input handling, so be careful ;-)\n");
    printf("It is recommended to redirect error output to some file\n\n");
    printf("Reading commands data (alcatest.cmd)...\n");
    load_commands("alcatest.cmd");

    printf("Opening, locking and initialising modem...\n");
    if (!modem_open()) {
        switch (modem_errno) {
            case ERR_MDM_LOCK:
                message(MSG_ERROR, "Modem locked!");
                exit(1);
                break;
            case ERR_MDM_OPEN:
                message(MSG_ERROR, "Modem can't be opened!");
                exit(2);
                break;
            default:
                message(MSG_ERROR, "Unknown error!");
                exit(5);
                break;
        }
    }
    modem_setup(); 

    if (!modem_init()) {
        // just try to close binary mode...
        printf ("First init failed, trying to close binary mode\n");
        alcatel_detach();
        alcatel_done();
        
        if (!modem_init()) {
            switch (modem_errno) {
                case ERR_MDM_PDU:
                    message(MSG_ERROR, "Failed selecting PDU mode!");
                    exit(4);
                    break;
                case ERR_MDM_AT:
                    message(MSG_ERROR, "Modem not reacting!");
                    exit(3);
                    break;
                default:
                    message(MSG_ERROR, "Unknown error!");
                    exit(5);
                    break;
            }
        }
    }
    printf("Phone information:\n");
    get_manufacturer(data,sizeof(data));
    printf ("Manufacturer:\t%s\n",data);
    
    get_model(data,sizeof(data));
    printf ("Model:\t\t%s\n",data);
    
    get_revision(data,sizeof(data));
    printf ("Revision:\t%s\n",data);
   
    printf ("Initialising binary mode\n");
    alcatel_init();
    printf (ALCATEST " ready, type ? for help\n");
    do {
        printf("alc>");
        scanf("%s", command);
        len = strlen(command);
        if ((len == 1 && (command[0] == 'h' || command[0] == '?')) || (strcmp(command, "help") == 0)) {
            printf (ALCATEST " help\n"
                    "=============\n\n"
                    "Builtin commands:\n\n"
                    "Name/alias + params | Description\n"
                    "---------------------------------\n"
                    "help/h/?            | this help\n"
                    "quit/q              | quit\n"
                    "info/i command      | information about loaded command\n"
                    "define              | define new command\n"
                    "read                | read packet and send ack\n"
                    "write data          | write raw packet\n"
                    "string data         | convert hex data to string\n"
                    "\nLoaded commands:\n"
                   );
            show_loaded_commands();
        } else if ((strcmp(command, "info") == 0) || (len == 1 && command[0] == 'i')) {
            char cmd[1024];
            scanf("%s", cmd);
            show_command_info(cmd);
        } else if (strcmp(command, "read") == 0) {
            read_data();
        } else if (strcmp(command, "string") == 0) {
            char cmd[1024];
            scanf("%s", cmd);
            show_string(cmd);
        } else if (strcmp(command, "write") == 0) {
            char cmd[1024];
            scanf("%s", cmd);
            write_data(cmd);
        } else if (strcmp(command, "define") == 0) {
            char  buff[MAX_LINE_LEN];
            
            fgets(buff, MAX_LINE_LEN-1, stdin);// empty buffer
            printf("Ented new command definition:\n");
            printf("Format: \"name\" \"description\" \"hex data + %%0-%%9 as params\" read_packets param_count {type \"description\" min_len max_len} ...\n");
            fgets(buff, MAX_LINE_LEN-1, stdin);
            if (!parse_cmd_line(buff))
                printf ("Bad command definition\n");
            else 
                printf ("Command defined\n");
        } else if ((strcmp(command, "quit") == 0) || (len == 1 && command[0] == 'q')) {
            len = 1;
        } else if (!do_command(command)) {
            printf ("Unhadled command: \"%s\"\n", command);
        }
    } while (!(len == 1 && command[0] == 'q')); 
    printf ("Closing binary mode\n");
    alcatel_done();
    printf ("Closing modem\n");
    modem_close();
            
    return 0;
}
