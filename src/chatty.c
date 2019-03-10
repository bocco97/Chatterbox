/** \file chatty.c  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/  
/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/select.h>
#include<rnwn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <set_config.h>
#include <stats.h>
#include <connections.h>
#include<icl_hash.h>
#include<queue.h>
#include<fd_queue.h>
#include<dataHandler.h>
#include<user.h>
//------------------------------------------------------------------------ VARIABILI GLOBALI -------------------------------------------------------------------------------------//
/* struttura che memorizza le statistiche del server, struct statistics 
 * e' definita in stats.h.
 *
 */

struct statistics  chattyStats = { 0,0,0,0,0,0,0 };

pthread_mutex_t statsLock[7];						//lock per l'accesso alle statistiche

struct Config c;									// struttura che contiene i parametri di configurazione

online_set onSet;									//set degli utenti attualmente online

icl_hash_t *ht;										//hash table che contiene tutti gli utenti registrati

fd_queue * q;										//coda dei descrittori

fd_queue * q2;										//coda dei descrittori su cui è stata elaborata una richiesta										

int stop_working;									//variabile di terminazione

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

/*
* Funzione di elaborazione del messaggio -- eseguita dai threads worker
*/
void process_request(message_t *msg,long fd){

	if(!msg) return ;
	message_t ans;
	memset(&ans,0,sizeof(ans));
	switch (msg->hdr.op){

		case REGISTER_OP:{
			char key[MAX_NAME_LENGTH+1];
			strncpy(key,msg->hdr.sender,MAX_NAME_LENGTH+1);
			pthread_mutex_lock(&onSet.lock);
			if(icl_hash_find(ht,key)!=NULL){																			//controllo che non esista già un utente con lo stesso nome
				pthread_mutex_unlock(&onSet.lock);
				ADD_ERROR (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_NICK_ALREADY,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_REGISTER_OP");
				return;
			} 
			else if(strlen(key)){
				icl_entry_t *new=NULL;
				new=icl_hash_insert(ht,key,NULL);    																	//aggiungo l'utente nella hash table
				if(!new){
					pthread_mutex_unlock(&onSet.lock);
					setHeader(&ans.hdr,OP_FAIL,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);
					if(x==-1 && errno!=EPIPE) perror("sendHeader_REGISTER_OP");
					perror("icl_hash_insert");
					return ;
				}
				else ADD_USER (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_OK,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_REGISTER_OP");
				int r=connect_user(&onSet,fd,msg->hdr.sender);															//connetto l'utente
        		if(r){
        			ADD_ERROR (statsLock,&chattyStats);
        			pthread_mutex_unlock(&onSet.lock);
        			setHeader(&ans.hdr,OP_FAIL,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);
					if(x==-1 && errno!=EPIPE) perror("sendHeader_REGISTER_OP");
       				return;
       			}
				int count=0;
				char *list=(char*)malloc(c.MaxConnections*(MAX_NAME_LENGTH+1)*sizeof(char));
				memset(list,0,c.MaxConnections*(MAX_NAME_LENGTH+1)*sizeof(char));
				for(int i=0; i<c.MaxConnections; i++){																	//invio la lista degli utenti online
					if(onSet.users[i].valid==1){
						count ++;
						strncpy(list+((count-1)*(MAX_NAME_LENGTH+1)),onSet.users[i].nickname,strlen(onSet.users[i].nickname)+1);
						for(int j=(((count-1)*(MAX_NAME_LENGTH+1))+strlen(onSet.users[i].nickname)); j<((count)*(MAX_NAME_LENGTH+1)); j++)
							list[j]='\0';						 
					}
				}
				pthread_mutex_unlock(&onSet.lock);
				list[count*(MAX_NAME_LENGTH+1)]='\0';
				setData(&ans.data,msg->hdr.sender,list,count*(MAX_NAME_LENGTH+1));
				int y=sendData(fd,&ans.data);
				if(y==-1 && errno!=EPIPE) perror("sendRequest_USRLIST_OP");
				if(list) free(list);
			}
			pthread_mutex_unlock(&onSet.lock);
		} break;

		case CONNECT_OP:{	
 			pthread_mutex_lock(&onSet.lock);
			if(!icl_hash_find(ht,msg->hdr.sender)){	
				pthread_mutex_unlock(&onSet.lock);																			//controllo che il mittente sia già registrato
     			ADD_ERROR (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_NICK_UNKNOWN,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
        		return;
        	}
        	int r=connect_user(&onSet,fd,msg->hdr.sender);																	//connetto l'utente
        	if(r){
        		pthread_mutex_unlock(&onSet.lock);
     			ADD_ERROR (statsLock,&chattyStats);	
				setHeader(&ans.hdr,OP_FAIL,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
       			return;
       		}
       		else {
				setHeader(&ans.hdr,OP_OK,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
				int count=0;
				char *list=(char*)malloc(c.MaxConnections*(MAX_NAME_LENGTH+1)*sizeof(char));
				memset(list,0,c.MaxConnections*(MAX_NAME_LENGTH+1)*sizeof(char));
				for(int i=0; i<c.MaxConnections; i++){																		//invio la lista degli utenti online
					if(onSet.users[i].valid==1){
						count ++;
						strncpy(list+((count-1)*(MAX_NAME_LENGTH+1)),onSet.users[i].nickname,strlen(onSet.users[i].nickname)+1);
						for(int j=(((count-1)*(MAX_NAME_LENGTH+1))+strlen(onSet.users[i].nickname)); j<((count)*(MAX_NAME_LENGTH+1)); j++)
							list[j]='\0';						 
					}
				}
				pthread_mutex_unlock(&onSet.lock);
				setData(&ans.data,msg->hdr.sender,list,count*(MAX_NAME_LENGTH+1));
				int y=sendData(fd,&ans.data);
				if(y==-1 && errno!=EPIPE) perror("sendRequest_USRLIST_OP");
				if(list) free(list);
       		}
       		pthread_mutex_unlock(&onSet.lock);
		}break;

		case POSTTXT_OP:{
			pthread_mutex_lock(&onSet.lock);
        	if(!is_online(&onSet,msg->hdr.sender)){																			//controllo che il mittente sia online
        		pthread_mutex_unlock(&onSet.lock);
     			ADD_ERROR (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_SENDER_OFFLINE,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
        		return;
        	}
        	if(msg->data.hdr.len>c.MaxMsgSize){																				//controllo che il messaggio rispetti la MaxSize
        		pthread_mutex_unlock(&onSet.lock);
				ADD_ERROR (statsLock,&chattyStats);
        		setHeader(&ans.hdr, OP_MSG_TOOLONG,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
        		return;
        	}
        	icl_entry_t *dest;
        	message_t *new=malloc(sizeof(message_t));
        	memset(new,0,sizeof(*new));
			new->hdr.op=TXT_MESSAGE;
			strncpy(new->hdr.sender,msg->hdr.sender,MAX_NAME_LENGTH+1);
			strncpy(new->data.hdr.receiver,msg->data.hdr.receiver,MAX_NAME_LENGTH+1);
			new->data.hdr.len=msg->data.hdr.len;
			new->data.buf=malloc((new->data.hdr.len+1)*sizeof(char));
			strncpy(new->data.buf,msg->data.buf,msg->data.hdr.len+1);
			int delivered=0;
        	if(is_online(&onSet,msg->data.hdr.receiver)){																	//invio il messaggio al destinatario se è online
        		int fd_dest=get_fd(&onSet,msg->data.hdr.receiver);
        		int ris=sendRequest(fd_dest,new);
        		if(ris<0 && errno!=EPIPE){
        			pthread_mutex_unlock(&onSet.lock);
        			perror("sendRequest_POSTTXT_OP");
        			setHeader(&ans.hdr,OP_FAIL,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);
					if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
        			return;
        		}
        		else if(ris>0) {
        			delivered=1;
        			pthread_mutex_unlock(&onSet.lock);
        		}
        	}
        	pthread_mutex_unlock(&onSet.lock);
        	dest=icl_hash_find(ht,msg->data.hdr.receiver);																	//controllo che il destinatario esista
        	if(!dest){
     			ADD_ERROR (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_RECEIVER_UNKNOWN,"CHATTY");															
				int x=sendHeader(fd,&ans.hdr);
				if(new->data.buf) free(new->data.buf);
				if(new) free(new);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
        		return;
        	}
        	else{
        		int ris=add_msg(ht,dest,new,c.MaxHistMsgs);																	//c.MaxHistMsgs per specificare la max size della history
        		if(ris!=0){
        			perror("add_msg");
        			setHeader(&ans.hdr,OP_FAIL,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);
					if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
        			return;
        		}
        		else{
        			if(delivered==0) ADD_MSG_UNDEL(statsLock,&chattyStats);
        			else ADD_MSG_DEL (statsLock,&chattyStats);
					setHeader(&ans.hdr,OP_OK,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);
					if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
        		}
        	}
		} break;

		case POSTTXTALL_OP:{
			pthread_mutex_lock(&onSet.lock);																			
			if(!is_online(&onSet,msg->hdr.sender)){																			//controllo che il mittente del messaggio sia online
				pthread_mutex_unlock(&onSet.lock);
     			ADD_ERROR (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_SENDER_OFFLINE,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXTALL_OP");
       			return;
       		}
       		if(msg->data.hdr.len>c.MaxMsgSize){																				//controllo che il messaggio rispetti la MaxSize
       			pthread_mutex_unlock(&onSet.lock);
     			ADD_ERROR (statsLock,&chattyStats);
        		setHeader(&ans.hdr, OP_MSG_TOOLONG,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
        		return;
        	}
        	message_t *new=malloc(sizeof(message_t));
        	memset(new,0,sizeof(*new));
			new->hdr.op=TXT_MESSAGE;
			strncpy(new->hdr.sender,msg->hdr.sender,MAX_NAME_LENGTH+1);
			strncpy(new->data.hdr.receiver,msg->data.hdr.receiver,MAX_NAME_LENGTH+1);
			new->data.hdr.len=msg->data.hdr.len;
			new->data.buf=malloc((new->data.hdr.len+1)*sizeof(char));
			strncpy(new->data.buf,msg->data.buf,msg->data.hdr.len+1);
			int delivered=0;
       		for(int j=0; j< onSet.max_online; j++){																			//invio il messaggio a tutti gli utenti online 
       			if(onSet.users[j].valid==1 && strncmp(msg->hdr.sender,onSet.users[j].nickname,MAX_NAME_LENGTH)){			//tranne al mittente stesso
       				int ris=sendRequest(onSet.users[j].fd,new);
       				if(ris<0 && errno!=EPIPE){
        				perror("sendRequest_POSTTXTALL_OP");
        			}
        			else if(ris>0){
        				delivered++;
        			}
       			}
       		}
       		pthread_mutex_unlock(&onSet.lock);																			
       		int notdelivered=0;
        	icl_entry_t *corr=NULL;
        	for(int i=0; i<N_BUCKETS; i++){
       			corr=ht->buckets[i];
       			while(corr!=NULL){
       				if(strncmp(msg->hdr.sender,(char*)corr->key,MAX_NAME_LENGTH)!=0){										//controllo che corr!=sender //non aggiungo alla history del mittente
       					message_t *news=malloc(sizeof(message_t));
        				memset(news,0,sizeof(*news));
						news->hdr.op=TXT_MESSAGE;
						strncpy(news->hdr.sender,msg->hdr.sender,MAX_NAME_LENGTH+1);
						strncpy(news->data.hdr.receiver,msg->data.hdr.receiver,MAX_NAME_LENGTH+1);
						news->data.hdr.len=msg->data.hdr.len;
						news->data.buf=malloc((news->data.hdr.len+1)*sizeof(char));
						strncpy(news->data.buf,msg->data.buf,msg->data.hdr.len+1);
       					int ris=add_msg(ht,corr,news,c.MaxHistMsgs);         
       					if(ris!=0){
       						perror("add_msg");
       						setHeader(&ans.hdr,OP_FAIL,"CHATTY");
							int x=sendHeader(fd,&ans.hdr);
							if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
       						return;
       					}
        				else{
        					notdelivered++;
        				}
        			}
        			corr=corr->next;
        		}
       		}
       		for(int j=0;j<delivered;j++) ADD_MSG_DEL (statsLock,&chattyStats);
       		for(int j=0;j<notdelivered;j++) ADD_MSG_UNDEL (statsLock,&chattyStats);
       		if(notdelivered >= delivered) {														
       			pthread_mutex_lock(&statsLock[3]);
       			chattyStats.nnotdelivered -=delivered;												//tolgo dai messaggi notdelivered il numero di invii che ho realizzato(delivered)
       			pthread_mutex_unlock(&statsLock[3]);
       		}
			setHeader(&ans.hdr,OP_OK,"CHATTY");
			int x=sendHeader(fd,&ans.hdr);
			if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXTALL_OP");
			if(new->data.buf) free(new->data.buf);
			if(new) free(new);
		} break;

		case POSTFILE_OP:{
			pthread_mutex_lock(&onSet.lock);
			if(!is_online(&onSet,msg->hdr.sender)){															//controllo che il mittente sia online
				pthread_mutex_unlock(&onSet.lock);
     			ADD_ERROR (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_FAIL,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTFILE_OP");
        		return;
        	}
        	if(msg->data.hdr.len>c.MaxMsgSize){																//controllo che il file rispetti la MaxSize
        		pthread_mutex_unlock(&onSet.lock);
				ADD_ERROR (statsLock,&chattyStats);
        		setHeader(&ans.hdr, OP_MSG_TOOLONG,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
        		return;
        	}
 			message_t *new=malloc(sizeof(message_t)); 														//creo il messaggio che conterrà il nome del file 
        	memset(new,0,sizeof(*new));
        	setHeader(&new->hdr,FILE_MESSAGE,msg->hdr.sender);
        	new->data.buf=malloc((c.MaxMsgSize+1)*sizeof(char));
        	strncpy(new->data.buf,msg->data.buf,c.MaxMsgSize+1);
        	new->data.hdr.len=strlen(msg->data.buf);
        	int count_slash=0;										
        	for(int i=0; i<new->data.hdr.len; i++){															// conto il numero di '/' nel path del file
        		if(new->data.buf[i]=='/') count_slash++;													// se #/ > 1 il nome del file contiene il path
        	}																								// sostituisco quindi gli '/' con '_' per memorizzare il file con un 
        	if(count_slash>1){																				// nome unico, che contenga comunque anche il path
        		for(int i=0; i<new->data.hdr.len; i++){			
        			if(new->data.buf[i]=='/'){
        				new->data.buf[i]='_';		
        				count_slash--;
        			}
        			if (count_slash==0) break;
        		}														
        		if(new->data.buf[0]=='.') new->data.buf[0]='_';			
        	}	
        	strncpy(new->data.hdr.receiver,msg->data.hdr.receiver,MAX_NAME_LENGTH+1);
        	int delivered=0;
        	if(is_online(&onSet,msg->data.hdr.receiver)){													//se il destinatario è online, invio subito il messaggio con il nome del file
        		int fd_dest=get_fd(&onSet,msg->data.hdr.receiver);											// prendo il fd del destinatario dall'onSet
        		int ris=sendRequest(fd_dest,new);
        		if(ris<0 && errno!=EPIPE){
        			perror("sendRequest_POSTTXT_OP");
        			pthread_mutex_unlock(&onSet.lock);
        			setHeader(&ans.hdr,OP_FAIL,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);
					if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
        			if(new->data.buf) free(new->data.buf);
        			if(new) free(new);
        			return;
        		}
        		else if(ris>0){
        			delivered=1;
        			pthread_mutex_unlock(&onSet.lock);
        		}
        	}
        	pthread_mutex_unlock(&onSet.lock);
        	icl_entry_t *dest;
        	dest=icl_hash_find(ht,msg->data.hdr.receiver);													//controllo che il destinatario esista
        	if(!dest){
				ADD_ERROR (statsLock,&chattyStats);	
				setHeader(&ans.hdr,OP_RECEIVER_UNKNOWN,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
        		if(new->data.buf) free(new->data.buf);
        		if(new) free(new);
        		return;
        	}
        	int ris=add_msg(ht,dest,new,c.MaxHistMsgs);														//aggiungo il messaggio con il nome del file alla history del destinatario								
        	if(ris!=0){
        		perror("add_msg");
        		setHeader(&ans.hdr,OP_FAIL,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
        		if(new->data.buf) free(new->data.buf);
        		if(new) free(new);
        		return;
        	}
        	if(delivered==0) ADD_FILE_UNDEL (statsLock,&chattyStats);
        	else ADD_FILE_DEL (statsLock,&chattyStats);
        	char *filename=malloc((c.MaxMsgSize+1+strlen(c.DirName)+1)*sizeof(char));
        	memset(filename,0,sizeof(*filename));
  			strncpy(filename,c.DirName,strlen(c.DirName)+1);
  			strncat(filename,"/",sizeof(char));
  			strncat(filename,new->data.buf,c.MaxMsgSize+1);
  			message_data_t *file_body=(message_data_t*)malloc(sizeof(message_data_t));
  			memset(file_body,0,sizeof(*file_body));
        	int y=readData(fd,file_body);																			//leggo il file
        	if(y==-1){
        		perror("readData_POSTFILE_OP");
        		setHeader(&ans.hdr,OP_FAIL,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
        		return;
        	}	
        	int fp;
        	fp=open(filename, O_CREAT | O_WRONLY,0666);		
        	if(fp<0){
        		perror("open_POST_FILE_OP");
				setHeader(&ans.hdr,OP_FAIL,"CHATTY");
     			ADD_ERROR (statsLock,&chattyStats);
				if(filename) free(filename);
				if(file_body && file_body->buf) free(file_body->buf);
				if(file_body) free(file_body); 
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTFILE_OP");
				return;
        	}
        	int write=writen(fp,file_body->buf,file_body->hdr.len);
        	if(write==-1){
        		perror("writen_POST_FILE_OP");
     			ADD_ERROR (statsLock,&chattyStats);
        		setHeader(&ans.hdr,OP_FAIL,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTFILE_OP");
        	}
        	else{
        		setHeader(&ans.hdr,OP_OK,"CHATTY");
				y=sendHeader(fd,&ans.hdr);
				if(y==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
			}
			if(fp)		 			close(fp);
			if(filename) 			free(filename);
			if(file_body->buf) 		free(file_body->buf);
			if(file_body)			free(file_body);
		} break;

		case GETFILE_OP:{
			icl_entry_t * getter=icl_hash_find(ht,msg->hdr.sender);													//cerco l'utente che richiede di scaricare il file 
			if(!getter){	
     			ADD_ERROR (statsLock,&chattyStats);														
				setHeader(&ans.hdr,OP_FAIL,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXTALL_OP");
				return;
			}
			if(is_in_history(ht,getter,msg->data.buf)){																//controllo che il file che vuole scaricare sia effettivamente nella sua history
				char *filename=malloc((strlen(c.DirName)+2+strlen(msg->data.buf))*sizeof(char)); 
				memset(filename,0,sizeof(*filename));
				strncpy(filename,c.DirName,strlen(c.DirName)+1);  
				strncat(filename,"/",sizeof(char));              
				strncat(filename,msg->data.buf,c.MaxMsgSize+1); 
				int fdes=open(filename,O_RDONLY);	
				if (fdes<0) {
					perror("open_GETFILE_OP");
					setHeader(&ans.hdr,OP_FAIL,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);			
					if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTFILE_OP"); 
					if(filename) free(filename);
					return;
	    	    }
	    		char  *mappedfile = NULL;
	    		mappedfile = mmap(NULL,c.MaxFileSize, PROT_READ,MAP_PRIVATE, fdes, 0);
	    		close(fdes);
	    		fdes=-1;
	    		if (mappedfile == MAP_FAILED) {
					perror("mmap");
					setHeader(&ans.hdr,OP_FAIL,"CHATTY");
					int x=sendHeader(fd,&ans.hdr);
					if(x==-1 && errno!=EPIPE) perror("sendHeader_CONNECT_OP");
					if(filename) free(filename);
					return ;
	    		}
				setHeader(&ans.hdr,OP_OK,"CHATTY");
				int y=sendHeader(fd,&ans.hdr);
				if(y==-1 && errno!=EPIPE) perror("sendHeader_GETFILE_OP");
	    	    setData(&ans.data, "", mappedfile,c.MaxFileSize);
	    	    y=sendData(fd,&ans.data);
				if (y==-1 && errno!=EPIPE ) { 
	    			perror("sendData_GETFILE_OP");
	    			munmap(mappedfile, c.MaxFileSize);
	    			if(filename) free(filename);
	  			  	return ;
				}
				munmap(mappedfile, c.MaxFileSize);
				if(filename) free(filename);
			}
			else{	
	     	    ADD_ERROR (statsLock,&chattyStats);																	//non ho trovato il file richiesto nella history
				setHeader(&ans.hdr,OP_FAIL,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXTALL_OP");
			}
		} break;

		case GETPREVMSGS_OP:{
			pthread_mutex_lock(&onSet.lock);
			if(!is_online(&onSet,msg->hdr.sender)){																	//controllo che il mittente sia online 
				pthread_mutex_unlock(&onSet.lock);
     			ADD_ERROR (statsLock,&chattyStats);	
				setHeader(&ans.hdr,OP_SENDER_OFFLINE,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXTALL_OP");
       			return;
       		}
			icl_entry_t *elem=NULL;
			elem=icl_hash_find(ht,msg->hdr.sender);																	//recupero la entry del mittente dalla hash table
			if(!elem){
     			ADD_ERROR (statsLock,&chattyStats);
				setHeader(&ans.hdr,OP_NICK_UNKNOWN,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
				return;
			}
			else{
				setHeader(&ans.hdr,OP_OK,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
			}
			Queue_t *qtmp=NULL;
			Node_t *corr=NULL;
			int index=(* ht->hash_function)(elem->key) % N_SECTIONS;
			LockHash(ht->qlock,index);
			int y;
			if(elem->data!=NULL) qtmp=(Queue_t*)elem->data;
			if(qtmp!=NULL) corr=(Node_t*) qtmp->head;				
			size_t count;
			memset(&count,0,sizeof(size_t));
			if(qtmp!=NULL) count=length(qtmp);																		//count prende la lunghezza della history richiesta
			else count=0;																							// (o 0 in caso la history sia vuota)
			ans.data.hdr.len=sizeof(size_t);
			ans.data.buf=(char*)&count;
			ans.data.buf[sizeof(size_t)]='\0';
			sendData(fd,&ans.data);																					//comunico il numero di messaggi che verranno inviati
			while(corr!=NULL){
				if(corr->data!=NULL){
					message_t *tmp=NULL;
					tmp=(message_t*)corr->data;
					y=sendRequest(fd,tmp);
					
					if(y==-1 && errno!=EPIPE){
						perror("sendRequest_GETPREVMSGS");
					}
				}
				corr=corr->next;
			}
			UnlockHash(ht->qlock,index);
			pthread_mutex_unlock(&onSet.lock);
		} break;

		case  USRLIST_OP:{
			pthread_mutex_lock(&onSet.lock);														
			if(!is_online(&onSet,msg->hdr.sender)){																	//controllo che il mittente sia online
				pthread_mutex_unlock(&onSet.lock);
				ADD_ERROR (statsLock,&chattyStats);	
				setHeader(&ans.hdr,OP_SENDER_OFFLINE,"CHATTY");
				int x=sendHeader(fd,&ans.hdr);
				if(x==-1 && errno!=EPIPE) perror("sendHeader_USRLIST_OP");
       			return;
       		}
			setHeader(&ans.hdr,OP_OK,"CHATTY");
			int x=sendHeader(fd,&ans.hdr);
			if(x==-1 && errno!=EPIPE) perror("sendHeader_POSTTXT_OP");
			int count=0;																							//count numero degli utenti online
			char *list=(char*)malloc(c.MaxConnections*(MAX_NAME_LENGTH+1)*sizeof(char));
			memset(list,0,c.MaxConnections*(MAX_NAME_LENGTH+1)*sizeof(char));
			for(int i=0; i<c.MaxConnections; i++){
				if(onSet.users[i].valid==1){
					count ++;
					strncpy(list+((count-1)*(MAX_NAME_LENGTH+1)),onSet.users[i].nickname,strlen(onSet.users[i].nickname)+1);
					for(int j=(((count-1)*(MAX_NAME_LENGTH+1))+strlen(onSet.users[i].nickname)); j<((count)*(MAX_NAME_LENGTH+1)); j++)
						list[j]='\0';						 
				}
			}
			pthread_mutex_unlock(&onSet.lock);
			setData(&ans.data,msg->hdr.sender,list,count*(MAX_NAME_LENGTH+1));
			int y=sendData(fd,&ans.data);
			if(y==-1 && errno!=EPIPE) perror("sendRequest_USRLIST_OP");	
			if(list) free(list);
		} break;

		case UNREGISTER_OP:{																		//UnRegister --> Disconnect//non possono rimanere utenti online ma DeRegistrati
			pthread_mutex_lock(&onSet.lock);
			disconnect_user(&onSet,msg->hdr.sender);
			pthread_mutex_unlock(&onSet.lock);
			int x= icl_hash_delete(ht, msg->hdr.sender, NULL, NULL);								//elimino l'utente che ne ha fatto richiesta dopo averlo disconnesso
			if(x==0) {
			DEL_USER (statsLock,&chattyStats);
			setHeader(&ans.hdr,OP_OK,"CHATTY");
			int x=sendHeader(fd,&ans.hdr);
			if(x==-1 && errno!=EPIPE) perror("sendHeader_UNREGISTER_OP");
			return;
			}
		} break;

		case DISCONNECT_OP:{
			pthread_mutex_lock(&onSet.lock);
			disconnect_user(&onSet,msg->hdr.sender);
			pthread_mutex_unlock(&onSet.lock);
			setHeader(&ans.hdr,OP_OK,"CHATTY");
			int x=sendHeader(fd,&ans.hdr);
			if(x==-1 && errno!=EPIPE) perror("sendHeader_DISCONNECT_OP");
			fflush(stdout);
			return;
		} break;

		default: {
			ADD_ERROR (statsLock,&chattyStats);
			setHeader(&ans.hdr,OP_FAIL,"CHATTY");
			int x=sendHeader(fd,&ans.hdr);
			if(x==-1 && errno!=EPIPE) perror("sendHeader_DEFAULT");
			fflush(stdout);
			return ;
    	} break;
	}
	return ;
}

/*
*Funzione che aggiorna aggiorna fd set
*/
int update(fd_set set,int fdmax){        
	for(int i=(fdmax);i>=0;--i)
		if (FD_ISSET(i, &set)) return i;
    return -1;
}

/*
* Funzione eseguita da tutti i thread worker del pool
*/
void* Start(){

	while(!stop_working){
		int fdes;
		fdes=(fd_pop(q));
		if(fdes==-1){;}
		else{
			message_t msg;
			memset(&msg,0,sizeof(msg));
			int x;
			x=readMsg(fdes,&msg);
			if(x==-1 && errno!=EPIPE && errno!=ECONNRESET){
				perror("readMsg_Start");
			}
			else if(x==0){
				disconnect_by_fd(&onSet,fdes);				
				int tmp=-fdes;
				fd_push(q2,tmp);
			}
			else {
				process_request(&msg,fdes);
				int tmp=fdes; 
				fd_push(q2,tmp); 
			}
	    	if(msg.data.buf)	free(msg.data.buf);
	    }
	}
	pthread_mutex_unlock(&onSet.lock);
	for(int i=0; i<7; i++) pthread_mutex_unlock(&statsLock[i]);
	for(int i=0; i<N_SECTIONS; i++) UnlockHash(ht->qlock,i);
	pthread_exit(NULL);
}


static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

/*
* Funzione eseguita dal thread listener 
*/
void * Listen(){

	int fd_sk, fd_c, fd_num=0, fd;
	fd_set set,rdset;
	struct sockaddr_un sa;
	memset(&sa, '0', sizeof(sa));	
	sa.sun_family=AF_UNIX;
	strncpy(sa.sun_path,c.UnixPath,UNIX_PATH_MAX+1);
	fd_sk=socket(AF_UNIX,SOCK_STREAM,0);
	if(fd_sk==-1){
		perror("socket");
		return (void*)-1;
	}
	if(bind(fd_sk,(struct sockaddr *)&sa,sizeof(sa))==-1){
		perror("bind");
		return (void*)-1;
	}
	if(listen(fd_sk,SOMAXCONN)==-1){
		perror("listen");
		return (void*)-1;
	}											
	if(fd_sk>fd_num) fd_num=fd_sk;	
	FD_ZERO(&set);
	FD_ZERO(&rdset);
	FD_SET(fd_sk,&set);
	fd_num=fd_sk;
	struct timeval time;
	int ready;
	while(!stop_working){
		rdset=set;
		time.tv_sec=0;
		time.tv_usec=100;
		ready=select(fd_num+1,&rdset,NULL,NULL,&time);		
	  	if(ready==-1){
	 	 	perror("select");
	  	}	  	
	  	else if(ready>0){
			for(fd=0; fd<=fd_num; fd++){
				if(FD_ISSET(fd,&rdset)){		
						if(fd==fd_sk){				
							fd_c=accept(fd_sk,NULL,0);
							FD_SET(fd_c,&set);						
							if(fd_c>fd_num) fd_num=fd_c;
						}
						else{
								int tmp;
								tmp=fd;
								FD_CLR(tmp,&set);
								fd_push(q,tmp);
								fd_num=update(set,fd_num);
						}
				}
			}
		}
		else if(ready==0){									
			while(fdq_length(q2)>0){
				int fdes=(fd_pop(q2));
				if(fdes<0){
					int tm=-fdes;
					if(FD_ISSET(tm,&set)) FD_CLR(tm,&set);
					fd_num=update(set,fd_num);
					close(tm);
				}
				else if(fdes>0){
					int tm=fdes;
					FD_SET(tm,&set);
					if(tm>fd_num) fd_num=tm;
				}
			}	
		}	
	}
	close(fd_sk);
	pthread_exit(NULL);
}


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

int main(int argc, char *argv[]) {

	if(argc!=3){
		usage(argv[0]);
		return -1;
	}	
	sigset_t sset;
	sigfillset(&sset);															//maschero tutti i segnali
	memset(&c,0,sizeof(c));
	FILE *f;
	f=fopen(argv[2],"r");
	if(!f){
		perror("Opening conffile");
		return -1;
	}
	set_configuration(&c,f);
	//print_config(c);															//stampa la configurazione estratta dal file di configurazione			 
	fclose(f); 					
	unlink(c.UnixPath);
	memset(&onSet,0,sizeof(onSet));
	int y=initialize_online_set(&onSet,c.MaxConnections);
	if(y){
		perror("initialize_online_set");
		return -1;
	}
	ht=icl_hash_create(NULL,NULL);												//creo ht //null e null perchè le funzioni da usare sono già in icl.c
	if(!ht){
		perror("icl_hash_create");
		return -1;
	}
	q=init_fd_queue();
	if(!q){
		perror("initiQueue");
		return -1;
	}
	q2=init_fd_queue();
	if(!q2){
		perror("initiQueue");
		return -1;
	}
	char *path=malloc((c.MaxMsgSize+1+strlen(c.DirName)+1)*sizeof(char));
    memset(path,0,sizeof(*path));
  	strncpy(path,c.DirName,strlen(c.DirName)+1);
  	int d=mkdir(path,0777);														//creo la directory dove memorizzare i file che gli utenti si scambiano (path dal file di config.)
  	if(d==-1 && errno!=EEXIST){
  		perror("mkdir");
  		return -1;
  	}
  	for (int k=0; k<7; k++) pthread_mutex_init(&statsLock[k],NULL);
	stop_working=0;															
	pthread_sigmask(SIG_SETMASK,&sset,NULL);
	pthread_t *th=(pthread_t*)malloc((c.ThreadsInPool)*sizeof(pthread_t));			
	for(int j=0;j<c.ThreadsInPool;j++){											//creo i threads worker
		if(pthread_create(&th[j],NULL,Start,NULL)!=0){
			perror("pthread_create");
			return -1;
		}				
	}
	pthread_t listener;															//creo il thread listener
	if(pthread_create(&listener,NULL,Listen,NULL)!=0){
		perror("pthread_create_listener");
			return -1;
	}
	int sig;
	while(!stop_working){
		sigwait(&sset,&sig);
		switch(sig){

			case SIGQUIT:
			case SIGTSTP:
			case SIGTERM:
			case SIGINT:{
				stop_working=1;
			} break;

			case SIGUSR1:{
				pthread_mutex_lock(&onSet.lock);
				int users=onSet.users_online;
				pthread_mutex_unlock(&onSet.lock);
				pthread_mutex_lock(&statsLock[1]);
				chattyStats.nonline=users;
				pthread_mutex_unlock(&statsLock[1]);
				FILE* fstat=fopen(c.StatFileName,"w+");
				if(!fstat){
					perror("fopen_SIGUSR1");
					break;	
				}
				int ris=printStats(fstat);
				if(ris==-1) perror("printStats");
				if(fstat) fclose(fstat);
			} break;
		}
	}
	pthread_join(listener,NULL);								//quando il listener termina, stop_working=1, quindi inserisco nella coda dei descrittori il valore -1 per far terinare i thread worker
	int term=-1;
	if(q) {
		for(int y=0;y<c.ThreadsInPool;y++)
			fd_push(q,term);
	}
	for(int j=0;j<c.ThreadsInPool;j++) 		{	pthread_join(th[j],NULL);}				
	if(ht) {
		int x=icl_hash_destroy(ht,NULL,NULL);
		if(x==-1){
			perror("icl_hash_destroy");
			return -1;
		}
	}
	if(q)	delete_fd_queue(q);
	if(q2)  delete_fd_queue(q2);
	if(path) free(path);
	if(th) free(th);
	delete_online_set(&onSet);
	unlink(c.UnixPath);
	exit(EXIT_SUCCESS);
}