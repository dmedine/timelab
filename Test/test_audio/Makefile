CC=gcc

CFLAGS = -g --freestanding -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC
INCLUDE = -I../lib/inc -I../modules/inc
LIBPATH =  -L/usr/lib -L/lib64 -L/usr/include -L/usr/local/include
LIBPATH += -L../lib/bin -L../modules/bin
LIBS = -lportaudio -lm -lpthread -ldl -ltimelab -ltlmods
OBJ_PATH = ./objs

SRCS = test.c

.PHONY: test

all: test #modules lib

#lib: 
#	$(MAKE) -C ../lib

#modules: 
#	$(MAKE) -C ../modules

test.o: test.c
	$(CC) -c -o $@ $^ $(CFLAGS) $(INCLUDE) $(LIBPATH) $(LIBS) 

test: test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBPATH) $(LIBS) 

clean:
	rm test.o
