CC = gcc

.PHONY: build clean

all: build 

build:
	$(MAKE) -C build

clean:		
	rm -f src/tl_main.o	
	rm -f bin/timlab
	$(MAKE) clean -C build	
