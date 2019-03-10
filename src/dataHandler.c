/** \file dataHandler.c  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/
#include <dataHandler.h>
#include<stdio.h>
#include<queue.h>
#include<config.h>


int add_msg(icl_hash_t *ht,icl_entry_t *dest, message_t* new, int max){

	if(!ht || !dest || !new) { return -1;}
	int index=(* ht->hash_function)(dest->key) % N_SECTIONS;
	LockHash(ht->qlock,index);
	if(dest->data==NULL){
		dest->data=initQueue();
		dest->histSize=0;
	}
	if(dest->histSize==max){
		message_t * tmp=NULL;
		tmp=(message_t*)pop(dest->data);
		if(tmp->data.buf) free(tmp->data.buf);
		if(tmp) free(tmp);
		dest->histSize-=1;
	}
	int x=push(dest->data,new);
	if(x){
		perror("push(add_msg)");
		UnlockHash(ht->qlock,index);
		return -1;
	}
	dest->histSize+=1;
	UnlockHash(ht->qlock,index);
	return 0;
}



int is_in_history(icl_hash_t* ht,icl_entry_t* getter, char* filename){

	if(!ht || !getter || !filename) return 0;
	int index=(* ht->hash_function)(getter->key) % N_SECTIONS;
	LockHash(ht->qlock,index);
	Queue_t * q=NULL;
	q=(Queue_t*)getter->data;
	Node_t *node=(Node_t*)q->head;
	while(node!=NULL){
		if(node->data!=NULL) {
			message_t * msg=(message_t*) node->data;
			if(!strncmp((char*)msg->data.buf,filename,strlen(filename)+1)){
				
				UnlockHash(ht->qlock,index);
				return 1;
			}
		}
		node=node->next;
	}
	UnlockHash(ht->qlock,index);
	return 0;
}
