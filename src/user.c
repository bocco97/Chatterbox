/** \file user.c 
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/

#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>
#include<user.h>
#include<string.h>


int connect_user(online_set* set,int fd, char* name){

	if(set==NULL || name==NULL ) return -1;
	for(int i=0; i<(set->max_online); i++){
		if(set->users[i].valid==1){
			if(!strncmp(name,set->users[i].nickname,MAX_NAME_LENGTH)){
				set->users[i].fd=fd;
				return 0;
			}
		}
	}
	if(set->users_online  <  set->max_online){
		for(int i=0; i<(set->max_online); i++){
			if(set->users[i].valid==0){
				set->users[i].valid=1;
				set->users[i].fd=fd;
				strncpy(set->users[i].nickname,name,MAX_NAME_LENGTH+1);
				set->users[i].nickname[MAX_NAME_LENGTH]='\0';
				set->users_online+=1;
				return 0;
			}
		}

	}	
	return -1;
}


void disconnect_user(online_set* set, char* name){
	if(set==NULL || name==NULL) return;
	for(int i=0; i<(set->max_online); i++){
		if(set->users[i].valid==1){
			if(!strncmp(set->users[i].nickname,name,MAX_NAME_LENGTH)){
				set->users[i].valid=0;
				set->users_online-=1;
				return;
			}
		}
	}
	return;
}


void disconnect_by_fd(online_set* set,int fd){

	if(set==NULL) return;
	for(int i=0; i<(set->max_online); i++){
		if(set->users[i].valid==1){
			if(fd==set->users[i].fd){
				set->users[i].valid=0;
				set->users_online-=1;
				pthread_mutex_unlock(&set->lock);
				return;
			}
		}
	}
	return;

}


int is_online(online_set* set, char* name){

	if(set==NULL || name==NULL) return 0;
	for(int i=0; i<(set->max_online); i++){
		if(set->users[i].valid==1 && !strncmp(set->users[i].nickname,name,MAX_NAME_LENGTH)){
			return 1;
		}
	}
	return 0;
}


int get_fd(online_set* set, char* name){

	if(set==NULL || name==NULL) return 0;
	for(int i=0; i<(set->max_online); i++){
		if(set->users[i].valid==1){
			if(!strncmp(set->users[i].nickname,name,MAX_NAME_LENGTH)){
				return set->users[i].fd;
			}
		}
	}
	return -1;
}



int initialize_online_set(online_set* set, int max_users){

	if(set==NULL || max_users<0) return -1;
	set->users=malloc(max_users*sizeof(User)); 
	if(!set->users) return -1;
	set->users_online=0;
	set->max_online=max_users;
	for(int i=0; i<max_users; i++)  set->users[i].valid=0;
	pthread_mutex_init(&set->lock,NULL);
	return 0;
}

void delete_online_set(online_set* set){
	if(set!=NULL && set->users!=NULL)	free(set->users);
}