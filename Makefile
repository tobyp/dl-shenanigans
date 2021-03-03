LDFLAGS=-ldl
LDFLAGS_MODULE=-lc
CFLAGS=-ggdb
CFLAGS_MODULE=$(CFLAGS) -fPIC

all: module.so host

module.so: module.o
	ld -shared -fPIC $(LDFLAGS_MODULE) -o $@ $^

module.o: module.c
	gcc $(CFLAGS_MODULE) -o $@ -c $^

host: host.o

clean:
	rm *.o *.so host
