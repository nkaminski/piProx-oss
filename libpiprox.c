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
#include "libpiprox.h"

/* Opens the device node specified as `dev` and initializes the internal state.

   Returns -1 on failure and 0 on success
 */
int piprox_open(piprox_state_t *st, const char *dev){
    /* initialize length to zero */
    st->card_data_len = 0;
    /* attempt to open the device node */
    st->fd = open(dev, O_RDONLY);
    if (!st->fd)
    {
        return -1;
    } else {
        return 0;
    }
}

/* Closes the device fd associated with the state `st` */
void piprox_close(piprox_state_t *st){
    if(st->fd > 0 ){
        close(st->fd);
    }
}

/* Prints the contentents of the internal state to the supplied fd.

    Return negative values on failure and the number of printed characters on success.
*/
int piprox_print(piprox_state_t *st, int fd){
    int i,rv, out_len;
    if((rv = dprintf(fd, "--piprox state: fd=%d, length=%zu--\nData:",st->fd,st->card_data_len)) < 0){
        return rv;
    }
    out_len = rv;
    for(i=0;i<st->card_data_len;i++){
        if((rv = dprintf(fd,"%x",st->card_data[i])) < 0){
            return rv;
        }
        out_len += rv;
        /* Last iteration */
        if(i+1 == st->card_data_len){
            if((rv = dprintf(fd,"\n")) < 0){
                return rv;
            }
            out_len += rv;
        }
    }
    return out_len;
}

/* Reads card data from the receiver into the internal state, blocks until a card is presented.
   Returns -1 on failure and the size of the data on success.
 */
ssize_t piprox_read(piprox_state_t *st){
    ssize_t len;
    if(st->fd < 0){
        return -1;
    }
    len = read(st->fd,st->card_data,MAX_CARD_BYTES);
    /* do not specify a -ve length in the internal state */
    if(len < 0){
        st->card_data_len = 0;
    }
    else {
        st->card_data_len = len;
    }
    return len;
}

/* Attempts to parse the card data stored within the internal state following the HID Corporate 1000 format.
   Parity bits will also be vaildated.

   Returns 1 on success, 0 if the data stored within the internal state is too small to store a HID corporate 1k card and
   -1, -2, and -3 if the first, second or third parity check fails
 */
int piprox_hidcorp1k_parse(piprox_state_t *st, piprox_hidcorp1k_t *res){
    int i,f=0;
    uint8_t paritya=0,parityb=0,parityc=0;
    if(st->card_data_len<5){
        return 0;
    }
    res->facility = (((uint16_t)st->card_data[4] << 11) | ((uint16_t)st->card_data[3] << 3) | (st->card_data[2] >> 5)) & 0x0FFF;
    res->cardnum = (((uint32_t)st->card_data[2] << 15) | ((uint32_t)st->card_data[1] << 7) | ((uint32_t)st->card_data[0] >> 1)) & 0x000FFFFF;
    /* Parity scheme (using ONE indexing, bit 1 is the MSB):
       Even  2, 3, 4, 6, 7, 9, 10,12,13,15,16,18,19,21,22,24,25,27,28,30,31,33,34
       Odd  35, 2, 3, 5, 6, 8, 9, 11,12,14,15,17,18,20,21,23,24,26,27,29,30,32,33
       Odd  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35

       Parity scheme (using ZERO indexing, bit 0 is the LSB):
       Even 1, 2, 4, 5, 7, 8, 10, 11, 13, 14, 16, 17, 19, 20, 22, 23, 25, 26, 28, 29, 31, 32, 33
       Odd  0, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18, 20, 21, 23, 24, 26, 27, 29, 30, 32, 33
       Odd  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34
    */

    paritya = GETBIT(st->card_data,33);
    for(i=1;i<34;i=i+3){
        paritya ^= GETBIT(st->card_data,i);
        paritya ^= GETBIT(st->card_data,i+1);

    }
    if(paritya == 1){
        f=-1;
        goto fail;
    }
    parityb = GETBIT(st->card_data,0);
    for(i=2;i<34;i=i+3){
        parityb ^= GETBIT(st->card_data,i);
        parityb ^= GETBIT(st->card_data,i+1);
    }
    if(parityb == 0){
        f=-2;
        goto fail;

    }
    for(i=0;i<35;i++){
        parityc ^= GETBIT(st->card_data,i);
    }
    if(parityc == 0){
        f=-3;
        goto fail;
    }
fail:	if(f < 0){
            return f;
        } else {
            return 1;
        }
}

