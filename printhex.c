// printhex, example userspace code for interfacing with piProx and decoding the HID Corporate 1000 format
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
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
void main(int argc, char **argv){
	int fd;
	uint8_t buf[8];
	ssize_t size;
	fd = open("/dev/prox", O_RDONLY);
	if (!fd)
	{
		fprintf(stderr, "Unable to open /dev/prox\n");
		return;
	}
	size = read(fd,buf,8);
	printf("%d Bytes read\n", (int)size);
	printf("Card data: %x %x %x %x %x\n", buf[4],buf[3],buf[2],buf[1],buf[0]);
	perror ("Dev node status: \n");
	if(size==5){
		uint16_t facility;
		uint32_t cardnum;
		uint8_t paritya=0,parityb=0,parityc=0,f=0;
		int i;
		printf("Attempting HID Corporate 1000 Decode (35 bits).....\n");
		facility = (((uint16_t)buf[4] << 11) | ((uint16_t)buf[3] << 3) | (buf[2] >> 5)) & 0x0FFF;
		cardnum = (((uint32_t)buf[2] << 15) | ((uint32_t)buf[1] << 7) | ((uint32_t)buf[0] >> 1)) & 0x000FFFFF;  
		for(i=2;i<35;i++){
			paritya += ((buf[i/8] >> (i % 8)) & 0x01);
			i++;
			paritya += ((buf[i/8] >> (i % 8)) & 0x01);
		}
		if((paritya % 2) == 1){
			f++;
			goto fail;

		}
		for(i=1;i<35;i++){
			parityb += ((buf[i/8] >> (i % 8)) & 0x01);
			i++;
			parityb += ((buf[i/8] >> (i % 8)) & 0x01);
		}
		if((parityb % 2) == 0){
			f++;
			goto fail;

		}
		for(i=0;i<35;i++){
			parityc += ((buf[i/8] >> (i % 8)) & 0x01);
		}
		if((parityc % 2) == 0){
			f++;			
			goto fail;
		}
		printf("Facility Code: %i, Card Number %lu\n", facility, cardnum);
fail:		if(f)
			printf("HID Corporate 1000 Decode Failed!\n");
	}		
	close(fd);
}
