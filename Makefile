# Makefile

all: compile

install: compile
	cp alctest/alctest /usr/local/bin
	cp settty/settty /usr/local/bin

clean:
	cd lib; make clean
	cd settty; make clean
	cd alctest; make clean

compile:
	cd lib ; make
	cd settty ; make
	cd alctest ; make
