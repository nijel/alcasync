# Makefile

CC_FLAGS = -Wall -I../lib
CC = g++ $(CC_FLAGS)

OBJ = ../lib/logging.o ../lib/modem.o ../lib/common.o ../lib/mobile_info.o ../lib/charset.o ../lib/pdu.o ../lib/sms.o ../lib/alcatel.o
HEAD = ../lib/alcatel.h ../lib/charset.h ../lib/common.h ../lib/logging.h ../lib/mobile_info.h ../lib/modem.h ../lib/pdu.h ../lib/sms.h ../lib/version.h

all: alcatest

alcatest: main.c $(OBJ) $(HEAD)
	$(CC) -o alcatest main.c $(OBJ)

clean:
	rm -f alcatest
