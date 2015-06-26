current: tl_linux
CC=gcc
NAME?=foo
LIBS?=

#--------------LINUX

tl_linux: $(NAME).tl_linux

.SUFFIXES: .tl_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch $(CFLAGS)

LINUXINCLUDE =  -I../inc

.c.tl_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	$(CC) $(LIBS) -export_dynamic -shared -o $*.tl_linux $*.o -lc -lm
	strip --strip-unneeded $*.tl_linux
	rm -f $*.o
