obj-m += piProx.o

all: modules printhex
 
modules: piProx.c
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules 
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	$(RM) printhex

printhex: libpiprox.c printhex.c
	$(CC) -o printhex libpiprox.c printhex.c

http-client: libpiprox.c http-client.c
	$(CC) -lcurl -o http-client libpiprox.c http-client.c
