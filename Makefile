obj-m += piProx.o

all: modules printhex
 
modules: piProx.c
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) printhex

printhex: libpiprox.c printhex.c
	$(CC) -o printhex libpiprox.c printhex.c
