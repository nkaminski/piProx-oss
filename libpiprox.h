// libpiprox, userspace library code for interfacing with piProx and decoding the HID Corporate 1000 format
// Copyright (C) 2017 Nash Kaminski
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

/* Global parameters */
#define MAX_CARD_BYTES 8
#define GETBIT(arr,x) ((arr[(x)/8] >> ((x)%8)) & 0x01)

/* Type declarations */

typedef struct {
	int fd;
	uint8_t card_data[MAX_CARD_BYTES];
	size_t card_data_len;
} piprox_state_t;

typedef struct {
	uint16_t facility;
	uint32_t cardnum;
} piprox_hidcorp1k_t;  

/* function protos */

int piprox_open(piprox_state_t*, const char*);
void piprox_close(piprox_state_t*);
ssize_t piprox_read(piprox_state_t*);
int piprox_print(piprox_state_t*, int);
int piprox_hidcorp1k_parse(piprox_state_t*, piprox_hidcorp1k_t*);
void piprox_close(piprox_state_t*);

