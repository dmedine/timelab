CC=gcc

NAME?=lr_test

INCLUDE=-I../build/inc

LIBPATH =-L../build  -L/lib64 -L/usr/include -L/usr/local/include -L/usr/lib
LIBS  =-ltimelab

#TL_OBJS=../build/objs/a_pa.o \
	../build/objs/g_api.o \
	../build/objs/m_modules.o \
	../build/objs/m_ode_prim.o \
	../build/objs/ui_main.o	\
	../build/objs/ui_scope.o \
	../build/objs/gl_scope.o

 current: tl

 tl: $(NAME).tl 

 .SUFFIXES: .tl

 CFLAGS = -g -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
     -Wall -W -Wshadow -Wstrict-prototypes \
     -Wno-unused -Wno-parentheses -Wno-switch

.c.tl:	 
	$(CC) -g -o $*.o  $(CFLAGS) $(INCLUDE) -c $*.c
	ld -export_dynamic  -shared -o $*.tl $*.o $(LIBS) \
	$(LIBPATH) $(LIBS) -lc -lm 
	strip --strip-unneeded $*.tl 
 clean:
	rm $(NAME).tl
	rm -f $(NAME).o
