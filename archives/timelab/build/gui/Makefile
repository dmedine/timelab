INCLUDE = -I../inc -I/usr/local/include
GUIINCLUDE += $(shell pkg-config --cflags gtk+-2.0)
GUIINCLUDE += $(shell pkg-config --cflags glfw3)
GUIINCLUDE += $(shell pkg-config --cflags gslu)

MOREFLAGS = -o

LIBPATH = -L/usr/lib -L/build -L/lib64 -L/usr/include -L/usr/local/include
LIBS = -lportaudio -lm -lpthread 
GUILIBS = -lgslu -ldl
GUILIBS += $(shell pkg-config --libs gtk+-2.0)
GUILIBS += $(shell pkg-config --libs glfw3)
GUILIBS += -lGLEW -lGL -lm -lgsl -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lrt

GUISRCS = ui_main.c\
	  gl_scope.c\
	  shader_utils.c


GUIOBJS = $(GUISRCS:.c=.o)

all: empty

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(GUIINCLUDE) $(LIBPATH) $(LIBS) $(GUILIBS) -c -o $@ $^

empty: $(GUIOBJS)
	$(CC) $(MOREFLAGS) $@ $^  $(CFLAGS) $(LIBPATH) $(LIBS)   
