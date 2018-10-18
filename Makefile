obj-m += piProx.o
UNAME := $(shell uname -r)

all: modules printhex
 
modules: piProx.c
	$(MAKE) -C /lib/modules/$(UNAME)/build M=$(CURDIR) modules 
clean:
	$(MAKE) -C /lib/modules/$(UNAME)/build M=$(CURDIR) clean
	$(RM) printhex

printhex: libpiprox.c printhex.c
	$(CC) -o printhex libpiprox.c printhex.c

http-client: libpiprox.c http-client.c
	$(CC) -lcurl -o http-client libpiprox.c http-client.c
