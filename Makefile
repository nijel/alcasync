# Makefile

all: compile

install: compile
	cp alctest/alctest /usr/local/bin

clean:
	cd lib; make clean
	cd alctest; make clean

compile:
	cd lib ; make
	cd alctest ; make
