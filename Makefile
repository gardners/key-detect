CC=cc
COPT=-Wall -g -O3
LOPT=

HEADERS=keydetect.h \
	sha2.h \
	Makefile

SOURCES=datastructure.c main.c sha2.c

all:	test

test:	$(HEADERS) $(SOURCES)
	$(CC) $(COPT) $(LOPT) -o test $(SOURCES)
