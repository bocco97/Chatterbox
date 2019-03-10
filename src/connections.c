/** \file connections.c  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/

#include<connections.h>
#include <message.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include<sys/un.h>
#include<sys/select.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<config.h>

#define UNIX_PATH_MAX  64

static inline int readn(long fd, void *buf, size_t size) {
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

static inline int writen(long fd, void *buf, size_t size) {
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


int openConnection(char* path, unsigned int ntimes, unsigned int secs){									

	if(ntimes<=0 || ntimes>10 || secs>3 || secs <0) return -1;

	struct sockaddr_un sa;
	strncpy(sa.sun_path,path,UNIX_PATH_MAX+1);
	sa.sun_family=AF_UNIX;
	int fd_skt=socket(AF_UNIX,SOCK_STREAM,0);
	for(int i=0; i<ntimes; i++){
		if(connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa))==-1) {  
			sleep(secs);
		}
		else return fd_skt;
	}

	return -1;
}


int readHeader(long connfd, message_hdr_t *hdr){

	//if(hdr==NULL) return 0;
	memset(hdr,0,sizeof(*hdr));
	
	int r = readn(connfd,&hdr->op,sizeof(op_t));
	if(r<0)	{
		perror("readn_op");
		return -1;
	}

	int v= readn(connfd,hdr->sender,MAX_NAME_LENGTH+1);
	
	if(v<0) {

		perror("readn_sender");
		return -1;
	}

	return r+v;
}


int readData(long fd, message_data_t *data){

	//if(data==NULL) return 0;
	memset(data,0,sizeof(*data));

	int r = readn(fd,data->hdr.receiver,sizeof(data->hdr.receiver));
	if(r<0) {
		perror("readn_receiver");
		return -1;
	}

	int m=readn(fd,&data->hdr.len,sizeof(data->hdr.len));
	if(m<0) {
		perror("readn_len");
		return -1;
	}	
	if(data->buf==NULL){
		data->buf=malloc((data->hdr.len+1)*sizeof(char));
		memset(data->buf,0,(data->hdr.len+1)*sizeof(char));
	}
	int w=0;
	if(data->hdr.len>0)  w=readn(fd,data->buf,data->hdr.len+1);

	if(w<0) {
		perror("readn_buf");
		return -1;
	}

	return r+m+w;
}


int readMsg(long fd, message_t *msg){

	//if(msg==NULL) return 0;

	memset(msg,0,sizeof(*msg));
	int r=readHeader(fd,&msg->hdr);
	if(r<0){
		perror("readHeader");
		return -1;
	}

	int w=readData(fd,&msg->data);
	if(w<0){

		perror("readData");
		return -1;
	}
	
	return r+w;

}


int sendHeader(long fd, message_hdr_t *msg){

	if (msg==NULL) return 0;
	int w=writen(fd,&msg->op,sizeof(op_t));

	if(w<0 && errno!=EPIPE){

		perror("writen_op");
		return -1;
	}

	w=writen(fd,msg->sender,MAX_NAME_LENGTH+1);
	if(w<0 && errno!=EPIPE){

		perror("writen_sender");
		return -1;
	}

	return w;
}

int sendData(long fd, message_data_t *msg){

	if(msg==NULL) return 0;

	if(msg->hdr.receiver!=NULL){
		int w=writen(fd,msg->hdr.receiver,MAX_NAME_LENGTH+1);
		if(w<0 && errno!=EPIPE){

			perror("writen_receiver");
			return -1;
		}

	w=writen(fd,&msg->hdr.len,sizeof(unsigned int));
	if(w<0 && errno!=EPIPE){

		perror("writen_len");
		return -1;
	}	
	if(msg->hdr.len>0){ 
		w=writen(fd,msg->buf,msg->hdr.len+1);
		if(w<0 && errno!=EPIPE){

			perror("writen_buf");
			return -1;
		}
	}
	}
	return 0;
}



int sendRequest(long fd, message_t *msg){

	if(msg==NULL) return 0;

	int w=sendHeader(fd,&msg->hdr);
	if(w<0 && errno!=EPIPE){

		perror("sendHeader");
		return -1;
	}

	w=sendData(fd,&msg->data);
	if(w<0 && errno!=EPIPE){

		perror("sendData");
		return -1;
	}	
	return w;
}
