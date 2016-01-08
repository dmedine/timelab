CC=gcc

vpath %.c src
vpath %.h inc

LOOP_CNT?=1
BATCH_MODE?=0
DEBUG?=0

CFLAGS = -DLOOPS='$(LOOP_CNT)' -DBATCH='$(BATCH_MODE)' -DBUG='$(DEBUG)'
CFLAGS += -g
CFLAGS += -ffreestanding -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch
INCLUDE = -Iinc -I/usr/local/include
INCLUDE += $(shell pkg-config --cflags gtk+-2.0)
INCLUDE += $(shell pkg-config --cflags glfw3)
INCLUDE += $(shell pkg-config --cflags gslu)

MOREFLAGS = -o

LIBPATH = -L/usr/lib -L/build -L/lib64 -L/usr/include -L/usr/local/include
LIBS = -lportaudio -lm -lpthread -ldl
LIBS += $(shell pkg-config --libs gtk+-2.0)
LIBS += $(shell pkg-config --libs glfw3)
LIBS += $(shell pkg-config --libs gslu)
LIBS += -lGLEW -lGL -lm -lgsl -lX11 -lXxf86vm -lXrandr -lXi -lrt


OBJ_PATH = objs

SRCS = a_pa.c\
	g_api.c\
	g_utils.c\
	ui_main.c\
	m_modules.c \
	m_ode_prim.c \
	gl_scope.c\
	shader_utils.c

#ui_scope.c\

OBJS = $(SRCS:.c=.o)

EXECSRCS = tl_main.c

EXECOBJS = $(EXECSRCS:.c=.o)


.PHONY: libtimelab.a timelab

all: libtimelab.a timelab

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBPATH) $(LIBS) -c -o $@ $^

libtimelab.a: $(OBJS)
	$(AR) -r $@ $(OBJS)


timelab: $(EXECOBJS) $(OBJS)
	$(CC) $(MOREFLAGS) $@ $^ $(CFLAGS) $(LIBPATH) $(LIBS)
	mv *.o $(OBJ_PATH)
	mv timelab ../bin/


clean:
	rm $(OBJ_PATH)/*.o
	rm *.a
	rm ../bin/timelab

