obj-m += piProx.o

all: modules printhex
 
modules: piProx.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm printhex

printhex: libpiprox.c printhex.c
	gcc -o printhex libpiprox.c printhex.c
