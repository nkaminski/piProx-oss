// piProx, a weigand proximity reader driver for the Raspberry Pi
// Copyright (C) 2013 Nash Kaminski
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/wait.h>

// Preprocessor defined constants
#define DRIVER_AUTHOR "Nash Kaminski"
#define DRIVER_DESC   "Weigand Proximity Reader Driver for Raspberry Pi"
#define MOD_VERSION "0.9.1"
#define SHORT_DESC "prox"
#define HIGH_INTERRUPT_PIN 18
#define LOW_INTERRUPT_PIN 25 
#define PIPROX_MAX_DATA_LEN 6

// text below will be seen in 'cat /proc/interrupt' command
#define HIGH_INTERRUPT_DESC "Weigand high interrupt"
#define LOW_INTERRUPT_DESC "Weigand low interrupt"
DECLARE_WAIT_QUEUE_HEAD(waitq);

short int piProx_high_irq  = 0, piProx_low_irq = 0, piProx_data_len = 0;
short int piProx_data_ready = 0, piProx_open=0;
static struct timer_list timer;
unsigned char inbuf[PIPROX_MAX_DATA_LEN]; 

//Major device number
static int major;
//uDev data structures
static struct class *piProx_class;
static struct device *piProx_device;

static size_t bits2bytes(short int bits){
	short int bytes = bits/8;
	if((bits % 8) > 0){
		return bytes+1;
	}
	return bytes;
}

//File operation functions
static int piProx_dev_open(struct inode *inode, struct file *file)
{
	//only one reader at a time
	if (piProx_open) return -EBUSY;
	piProx_open++;
	return 0;
}

static int piProx_dev_release(struct inode *inode, struct file *file)
{
	piProx_open--;
	return 0;
}

static ssize_t piProx_dev_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	short int to_copy;
	size_t nbytes;
	if (filp->f_flags & O_NONBLOCK)
	{
		//Nonblocking read requested
		if(piProx_data_ready){
			goto data;
		}
		return -EAGAIN;
	}
	//Blocking read requested
	if (wait_event_interruptible(waitq, piProx_data_ready == 1))
	{
		/* signal interrupted the wait, return */
		return -ERESTARTSYS;
	}
data:
//calculate number of bytes to return
	nbytes = bits2bytes(piProx_data_len);
	to_copy = min(len, nbytes);
	//printk( "piProx: Copying %d bytes of card data to userland\n", (int)nbytes);
	if(copy_to_user(buf, inbuf, to_copy)!=0){
		printk(KERN_NOTICE "piProx: Unable to copy data to userland\n");
	}
	if (!(filp->f_flags & O_NONBLOCK))
	{
		// Clear data buffer if call was made in blocking mode
		piProx_data_ready = 0;	
		piProx_data_len=0;
		memset(inbuf, (char)0, PIPROX_MAX_DATA_LEN);

	}	
	return to_copy;
}

//pointers to functions for file ops
static struct file_operations f = { .read = piProx_dev_read, .open = piProx_dev_open, .release = piProx_dev_release };

// initializes character device
int piProx_init_devnode(void)
{
	void *ptr_err;
	//Register actual character device
	if ((major = register_chrdev(0, SHORT_DESC, &f)) < 0)
		return major;
	//Create udev class
	piProx_class = class_create(THIS_MODULE, SHORT_DESC);
	if (IS_ERR(ptr_err = piProx_class))
		goto err2;
	//Create device file via udev
	piProx_device = device_create(piProx_class, NULL, MKDEV(major, 0), NULL, SHORT_DESC);
	if (IS_ERR(ptr_err = piProx_device))
		goto err;

	
	return 0;
// Handle errors by undoing whatever has been done
err:
	class_destroy(piProx_class);
err2:
	unregister_chrdev(major, SHORT_DESC);
	return PTR_ERR(ptr_err);
}

//Cleans up character device on exit/unload
void piProx_cleanup_devnode(void)
{
	device_destroy(piProx_class, MKDEV(major, 0));
	class_destroy(piProx_class);
	return unregister_chrdev(major, SHORT_DESC);
}

//Reverses order of bytes in an array
static void byte_reverse(char *data, size_t length){
	size_t a,b;
	char tmp;
	a=0;
	b=length-1;
	while((a-b)>1){
		tmp=data[a];
		data[a]=data[b];
		data[b]=tmp;
		a++;
		b--;
	}
}

//Reverses order of bits in byte
static char bit_reverse(char b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

//Logically (w/o sign extend) shifts bits in a byte array to the right
static void lsr_array(unsigned char *data, size_t length, unsigned short int shift){
	int i;
	if(length<1)
		return;
	for(i=0; i<length-1; i++){
		data[i]=(data[i] >> shift) | (data[i+1] << (8-shift));
	}
	data[length-1]=(data[length-1] >> shift);
}

//Sets a bit in a byte array
static void bit_set(char *buf, short int loc){
	short int byte = loc / 8;
	char bit = loc % 8;
	buf[byte] |= (1<<bit);	
}
// ISR Functions
void piProx_timer_callback( unsigned long data )
{
	size_t bytes;
	int i;
	//printk(KERN_NOTICE "piProx: Timer expired at %ld, %d bits read\n", jiffies, piProx_data_len);
	bytes=bits2bytes(piProx_data_len);
	for(i=0; i<bytes; i++){
		inbuf[i]=bit_reverse(inbuf[i]);
	}
	byte_reverse(inbuf,bytes);
	if((piProx_data_len % 8) != 0){ 
	lsr_array(inbuf,bytes,8-(piProx_data_len % 8));
	}
	piProx_data_ready = 1;
	wake_up_interruptible(&waitq);
}

static irqreturn_t high_ISR(int irq, void *dev_id, struct pt_regs *regs) {

	unsigned long flags;

	// disable hard interrupts (remember them in flag 'flags')
	local_irq_save(flags);
	//This is a new card, clear buffer
	if(piProx_data_ready){
		piProx_data_len=0;
		memset(inbuf, (char)0,PIPROX_MAX_DATA_LEN);
	}
	// Clear data ready and reset timeout
	piProx_data_ready = 0;
	mod_timer( &timer, jiffies + msecs_to_jiffies(20) );

	//Do not attempt to store more data than buffer can hold
	if(piProx_data_len < PIPROX_MAX_DATA_LEN * 8){
		bit_set((char *)inbuf,piProx_data_len);
		piProx_data_len++;
	}
	//printk(KERN_NOTICE "piProx: High interrupt [%d] for device %s was triggered !.\n", irq, (char *) dev_id);

	// restore hard interrupts
	local_irq_restore(flags);

	return IRQ_HANDLED;
}
// Same as high ISR but without bit set
static irqreturn_t low_ISR(int irq, void *dev_id, struct pt_regs *regs) {

	unsigned long flags;
	// disable hard interrupts (remember them in flag 'flags')
	local_irq_save(flags);
	if(piProx_data_ready){
		piProx_data_len=0;
		memset(inbuf, (char)0, PIPROX_MAX_DATA_LEN);
	}
	piProx_data_ready = 0;
	mod_timer( &timer, jiffies + msecs_to_jiffies(20) );
	if(piProx_data_len < PIPROX_MAX_DATA_LEN * 8){
		piProx_data_len++;
	}
	//printk(KERN_NOTICE "piProx: Low interrupt [%d] for device %s was triggered !.\n",irq, (char *) dev_id);

	// restore hard interrupts
	local_irq_restore(flags);

	return IRQ_HANDLED;
}

//Initializes timer
void piProx_timer_init(void){
	setup_timer( &timer, piProx_timer_callback, 0 );
	printk(KERN_NOTICE "piProx: Starting timer\n");
}

//Sets up a GPIO pin change interrupt 
short int attachInterrupt(int pin, const char *desc, const char *devdesc, int trigger, void *m_ISR){
	short int irq;
	//Request GPIO pin
	if (gpio_request(pin, desc)) {
		printk(KERN_NOTICE "piProx: GPIO request faiure: %s\n", desc);
		return -1;
	}
	//Set direction to input
	if (gpio_direction_input(pin)){
		printk(KERN_NOTICE "piProx: Failed to set GPIO direction");
		return -1;
	}
	//Get IRQ corresponding to that GPIO pin
	if ( (irq = gpio_to_irq(pin)) < 0 ) {
		printk(KERN_NOTICE "piProx: GPIO to IRQ mapping faiure %s\n", desc);
		return -1;
	}

	printk(KERN_NOTICE "piProx: Mapped GPIO# %d to HW IRQ, registering IRQ...%d\n", pin ,irq);
	// Actually register the IRQ
	// RISING, FALLING, HIGH, and LOW supported as trigger, precede with IRQF_TRIGGER_

	if (request_irq(irq,
				(irq_handler_t ) m_ISR,
				trigger,
				desc,
				(char *)devdesc)) {
		printk(KERN_NOTICE "piProx: IRQ Request failure\n");
		return -1;
	}

	return irq;
}

//Setup interrupts for input pins
void piProx_irq_config(void) {
	piProx_high_irq = attachInterrupt(HIGH_INTERRUPT_PIN, HIGH_INTERRUPT_DESC, NULL, IRQF_TRIGGER_FALLING, high_ISR);
	piProx_low_irq = attachInterrupt(LOW_INTERRUPT_PIN, LOW_INTERRUPT_DESC, NULL, IRQF_TRIGGER_FALLING, low_ISR);
}

//Detach interrupts and free GPIO pins on exit/unload
void piProx_irq_release(int pin, short int irq, const char *devdesc) {

	free_irq(irq, (void *)devdesc);
	gpio_free(pin);
	return;
}

//Called on insert/load, initializes everything via initializer function and checks for error in the process
int piProx_init(void) {
	printk(KERN_NOTICE "piProx: Module version " MOD_VERSION ", initializing...\n");
	piProx_irq_config();
	if(piProx_high_irq <=0 || piProx_low_irq <=0)
	  goto error1;
	if(piProx_init_devnode() != 0)
	  goto error2;
	piProx_timer_init();
	return 0;
// Make sure to not leak resources by cleaning up anything that was setup if an error occurs.
	error2:
	piProx_cleanup_devnode();
	error1:
	if(piProx_high_irq)
                piProx_irq_release(HIGH_INTERRUPT_PIN,piProx_high_irq,NULL);
        if(piProx_low_irq)   
                piProx_irq_release(LOW_INTERRUPT_PIN,piProx_low_irq,NULL);
	//Return code for IO error if interrupts of chardev cannot be created
	return -EIO;
}

//Main cleanup function, cleans up and frees all resources.
//Kernel has already ensured that nobody is accessing our device.
void piProx_cleanup(void) {
	printk(KERN_NOTICE "piProx: Exiting...\n");
	del_timer( &timer );
	piProx_cleanup_devnode();
	if(piProx_high_irq)
		piProx_irq_release(HIGH_INTERRUPT_PIN,piProx_high_irq,NULL);
	if(piProx_low_irq)   
		piProx_irq_release(LOW_INTERRUPT_PIN,piProx_low_irq,NULL);
	return;
}

// Kernel functions that must be supplied with a modules init and exit function pointers
module_init(piProx_init);
module_exit(piProx_cleanup);


/****************************************************************************/
/* Module licensing/description block.                                      */
/****************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
