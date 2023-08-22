CC=gcc
CCFLAGS=-O2 -Wall

LDLIBS=-lcfitsio -lm

all: tfits
tfits: tfits.c
	${CC} ${CCFLAGS} -o $@ $< ${LDLIBS}

install:tfits
	cp tfits /usr/bin/tfits

uninstall:
	rm /usr/bin/tfits

clean:
	@rm tfits

debug: CCFLAGS+=-g
debug:all
