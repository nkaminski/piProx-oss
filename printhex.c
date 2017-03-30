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
#define GETBIT(arr,x) ((arr[(x)/8] >> ((x)%8)) & 0x01)
int main(int argc, char **argv){
	int fd;
	int f=0;
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
		uint8_t paritya=0,parityb=0,parityc=0;
		int i;
		printf("Attempting HID Corporate 1000 Decode (35 bits).....\n");
		facility = (((uint16_t)buf[4] << 11) | ((uint16_t)buf[3] << 3) | (buf[2] >> 5)) & 0x0FFF;
		cardnum = (((uint32_t)buf[2] << 15) | ((uint32_t)buf[1] << 7) | ((uint32_t)buf[0] >> 1)) & 0x000FFFFF;  
		/* Parity scheme (using ONE indexing):
		Even  2  3, 4, 6, 7, 9, 10,12,13,15,16,18,19,21,22,24,25,27,28,30,31,33,34
		Odd  35 2, 3, 5, 6, 8, 9, 11,12,14,15,17,18,20,21,23,24,26,27,29,30,32,33
		Odd  1  2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35
		*/
		paritya = GETBIT(buf,1);
		for(i=2;i<34;i=i+3){
			paritya ^= GETBIT(buf,i);
			paritya ^= GETBIT(buf,i+1);
			
		}
		if((paritya % 2) == 1){
			f++;
			fprintf(stderr,"Parity check 1 failed!\n");
			goto fail;
		}
		parityb = GETBIT(buf,34);
		for(i=1;i<34;i=i+3){
			parityb ^= GETBIT(buf,i);
			parityb ^= GETBIT(buf,i+1);
		}
		if((parityb % 2) == 0){
			f++;
			fprintf(stderr,"Parity check 2 failed!\n");
			goto fail;

		}
		for(i=0;i<35;i++){
			parityc ^= GETBIT(buf,i);
		}
		if((parityc % 2) == 0){
			f++;
			fprintf(stderr,"Parity check 3 failed!\n");	
			goto fail;
		}
		printf("Facility Code: %i, Card Number %lu\n", facility, cardnum);
fail:		if(f)
			printf("HID Corporate 1000 Decode Failed!\n");
	}		
	close(fd);
	if(f){
		return 1;
	} else {
		return 0;
	}
}
