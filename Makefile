# Makefile

all: compile

install: compile
	cp alcademo/alcademo /usr/local/bin
	cp settty/settty /usr/local/bin

clean:
	cd lib; make clean
	cd settty; make clean
	cd alcademo; make clean

compile:
	cd lib ; make
	cd settty ; make
	cd alcademo ; make
