// http-client, userspace code for interfacing with piProx and HTTP POSTing the data to a specified endpoint
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
#include <curl/curl.h>
 
int main(int argc, char **argv){
    const char devnode[] = "/dev/prox";
    piprox_state_t prox;
    piprox_hidcorp1k_t hidcorp1k;
    int rv;
	char post_str[1024];
    CURL *curl;
	CURLcode res;
 
	if(argc != 2){
		printf("Usage: %s <post_url>\n",argv[0]);
		return 2;
	}
	/* In windows, this will init the winsock stuff */ 
	curl_global_init(CURL_GLOBAL_ALL);
 
	/* get a curl handle and initialize the URL*/ 
	curl = curl_easy_init();
	if(!curl){
        curl_global_cleanup();
		fprintf(stderr,"Failed to initialize cURL");
		return 1;
	}
    curl_easy_setopt(curl, CURLOPT_URL, argv[1]);

    /* open the device node and initialize the internal state */
    rv = piprox_open(&prox, devnode);
    if(rv < 0){
        fprintf(stderr, "piprox_open failed, return code is %d\n", rv);
        return 1;
    }
    while(1){
        printf("Waiting for card...\n");
        /* Read a card */
        rv = piprox_read(&prox);
        printf("Read %d bytes from reader\n", rv);
        if(rv > 0){
            /* If the read was successful, attempt a corp1k decode and http post the data if succesful*/
            rv = piprox_hidcorp1k_parse(&prox, &hidcorp1k);
            if(rv == 0){
                printf("Card data is smaller than 5 bytes\n");
            } else if( rv < 0){
                printf("Parity error on parity check %d in HID Corp. 1000 decoding\n", -rv);
            } else{
                printf("HID Corporate 1000 decoding done, facility=%d cardnum=%d\n", hidcorp1k.facility, hidcorp1k.cardnum);
                snprintf(post_str,sizeof(post_str),"type=hidcorp1k&facility=%d&cardnum=%d", hidcorp1k.facility, hidcorp1k.cardnum);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_str);
				/* Perform the request, res will get the return code */ 
				res = curl_easy_perform(curl);
				/* Check for errors */ 
				if(res != CURLE_OK)
					fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
            }
        }
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
