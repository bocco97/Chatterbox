/** \file rnwn.h  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/
#ifndef RNWN_H_
#define RNWN_H_
#include<stdlib.h>

int readn(long fd, void *buf, size_t size);

int writen(long fd, void *buf, size_t size);

#endif