CC=gcc

vpath %.c src
vpath %.h inc


CFLAGS = -fPIC
INCLUDE = -Iinc
LIBPATH = -L/usr/lib -L/lib64 -L/usr/include -L/usr/local/include
LIBS = -lportaudio -lm -lpthread #-ltimelab
OBJ_PATH = ./objs

#SRCS = tl_sig.c tl_ctl.c tl_core.c tl_pclass.c a_portaudio.c tl_dac-adc.c
SRCS = tl_core.c a_portaudio.c tl_sig.c tl_ctl.c tl_class_lists.c  tl_init_exit.c tl_args.c m_modules.c

OBJS = $(SRCS:.c=.o)


#.PHONY: libtimelab.a 
.PHONY: libtimelab.so

all: libtimelab.a libtimelab.so
#all: libtimelab.so

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
	$(CC) -shared -g3 -fPIC -Wl,-soname,libtimelab.so \
	-o libtimelab.so $(OBJS) -lc
	mv libtimelab.so ./bin
	mv *.o $(OBJ_PATH)

clean:
	rm $(OBJ_PATH)/*.o
	rm bin/*.a
