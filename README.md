# piProx-oss
Linux kernel driver and associated userland tools for interfacing wiegand based RFID readers to the Raspberry Pi via the GPIO pins. Also included is an Arduino implementation of the weigand signaling protocol to allow for readers to allow for USB or serial interfacing.

./reader/ contains the Arduino Wiegand implementation

piProx Usage:
	
	Build module and userspace utility by running:
		make all
	
	Insert compiled module by running (as root):
		insmod piProx.ko
		
	Reads from /dev/prox will normally block until a card is presented. In nonblocking mode, the previous card data 			will be returned or an error will be returned if no card has been presented since the last nonblocking read.
	
printhex Usage:
	Simply run ./printhex and present a card. The data returned from the card will be displayed along with (if the card 	is a Corporate 1000 card) the facility code and user ID.
