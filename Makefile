# Makefile

all: compile

install: compile
	cp alcademo/alcademo /usr/local/bin
# we don't install these because they are much more development tools than
# useable applications
#	cp alcatest/alcatest /usr/local/bin
#	cp settty/settty /usr/local/bin

clean:
	cd lib && make clean
	cd settty && make clean
	cd alcademo && make clean
	cd alcatest && make clean

compile:
	cd lib && make
	cd settty && make
	cd alcademo && make
	cd alcatest && make
    
tags: $(shell find . -name '*.[ch]' -o -name '*.cpp')
	ctags $(shell find . -name '*.[ch]' -o -name '*.cpp')
