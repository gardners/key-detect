CC=cc
COPT=-Wall -g -Ofast
LOPT=

HEADERS=keydetect.h \
	sha2.h \
	arithmetic.h \
	Makefile

SOURCES=datastructure.c main.c sha2.c arithmetic.c gsinterpolative.c

all:	test

test:	$(HEADERS) $(SOURCES)
	$(CC) $(COPT) $(LOPT) -o test $(SOURCES) -lz
