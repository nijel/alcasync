# Makefile

CC_FLAGS = -Wall
CC = g++-3.2 $(CC_FLAGS)
CXX = g++-3.2 $(CC_FLAGS)

OBJ = logging.o modem.o common.o mobile_info.o charset.o pdu.o sms.o alcatel.o

all: $(OBJ)

clean:
	rm -f *.o

$(OBJ): version.h modem.h
