# Makefile

VERSION=$(shell grep ALCASYNC_VERSION lib/version.h | sed 's/.*"\([0-9.]*\)[^0-9"]*".*/\1/')
ROOTDIR=$(shell pwd)

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
	rm -f tags

compile:
	cd lib && make
	cd settty && make
	cd alcademo && make
	cd alcatest && make
    
tags: $(shell find . -name '*.[ch]' -o -name '*.cpp')
	ctags $(shell find . -name '*.[ch]' -o -name '*.cpp')


dist: clean
	rm -rf /tmp/alcasync-${VERSION}
	mkdir /tmp/alcasync-${VERSION}
	cp -a . /tmp/alcasync-${VERSION}
	find /tmp/alcasync-${VERSION} \( -name CVS -o -name .\*.swp -o -name alcatest.log \) -print0 \
		| xargs -0 rm -rf
	tar cfz ${ROOTDIR}/../alcasync-${VERSION}.tar.gz -C /tmp alcasync-${VERSION}
