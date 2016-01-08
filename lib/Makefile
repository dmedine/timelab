CC=gcc

vpath %.c src
vpath %.h inc


CFLAGS = -fPIC
INCLUDE = -Iinc -I/usr/include -I/usr/local/include
LIBPATH = -L/usr/lib -L/lib64 
LIBS = -lportaudio -lm -lpthread -ldl
OBJ_PATH = ./objs

SRCS =  a_portaudio.c tl_sig.c tl_ctl.c tl_class_lists.c  tl_init_exit.c tl_args.c m_modules.c tl_core.c

OBJS = $(SRCS:.c=.o)

.PHONY: libtimelab.so

all: libtimelab.a libtimelab.so

debug: $(CFLAGS) += -DDEBUG -O0 -g3 -DPD
debug: all
release: $(CFLAGS) += -DTESTING -O2 -DPD -funroll-loops -fomit-frame-pointer
release: all

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBPATH) $(LIBS) -c -o $@ $^

libtimelab.a: $(OBJS)
	$(AR) -r $@ $(OBJS)
	mv libtimelab.a ./bin
	#mv *.o $(OBJ_PATH)

libtimelab.so: $(OBJS)
	$(CC) -shared -Wl,-soname,libtimelab.so \
	-Wl,--no-undefined $(OBJS) -lc -lportaudio -ldl -lm -lpthread \
	-o libtimelab.so
	mv libtimelab.so ./bin
	mv *.o $(OBJ_PATH)

clean:
	rm $(OBJ_PATH)/*.o
	rm bin/*.a
