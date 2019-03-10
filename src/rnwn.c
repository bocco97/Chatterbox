/** \file rnwn.c  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/
#include<errno.h>
#include<stdio.h>
#include<unistd.h>

#include<rnwn.h>

int readn(long fd, void *buf,size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	  	if (errno==ECONNRESET) return 0;
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return size;
}


int writen(long fd, void *buf,size_t size) {
    size_t left = size;
    int r=0;
    char *bufptr=NULL;
    bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
		if (errno==EPIPE) return 0;
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}