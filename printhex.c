// printhex, example userspace code for interfacing with piProx and decoding the HID Corporate 1000 format
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
#include "libpiprox.h"
int main(int argc, char **argv){
    const char devnode[] = "/dev/prox";
    piprox_state_t prox;
    piprox_hidcorp1k_t hidcorp1k;
    int rv;
    /* open the device node and initialize the internal state */
    rv = piprox_open(&prox, devnode);
    if(rv < 0){
        fprintf(stderr, "piprox_open failed, return code is %d\n", rv);
    }
    
    while(1){
        /* Read a card */
        rv = piprox_read(&prox);
        printf("Read %d bytes from reader\n", rv);
        if(rv > 0){
            /* If the read was successful, print the data and attempt a corp1k decode */
            piprox_print(&prox, stdout);
            rv = piprox_hidcorp1k_parse(&prox, &hidcorp1k);
            if(rv == 0){
                printf("Card data is smaller than 5 bytes\n");
            } else if( rv < 0){
                printf("Parity error on parity check %d in HID Corp. 1000 decoding\n", -rv);
            } else{
                printf("HID Corporate 1000 decoding done, facility=%d cardnum=%d\n", hidcorp1k->facility, hidcorp1k->cardnum);
            }
        }

