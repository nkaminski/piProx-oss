obj-m += piProx.o
 
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm printhex

printhex:
	gcc -o printhex printhex.c
