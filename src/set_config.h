/** \file set_config.h  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct Config{
	char UnixPath[100];
	char DirName[100];
	char StatFileName[100];
	unsigned long MaxConnections;
	unsigned long ThreadsInPool;
	unsigned long MaxMsgSize;
	unsigned long MaxFileSize;
	unsigned long MaxHistMsgs;

};

void print_config(struct Config conf){

	printf(" UnixPath:		%s \n",conf.UnixPath);
	printf(" DirName:		%s \n",conf.DirName);
	printf(" StatFileName:		%s \n",conf.StatFileName);
	printf(" MaxConnections:	%d \n",(int)conf.MaxConnections);
	printf(" ThreadsInPool:		%d \n",(int)conf.ThreadsInPool);
	printf(" MaxMsgSize:		%d \n",(int)conf.MaxMsgSize);
	printf(" MaxFileSize:		%d \n",(int)conf.MaxFileSize);
	printf(" MaxHistMsgs:		%d \n",(int)conf.MaxHistMsgs);

}

void put(struct Config* conf, char* field, char* value){
	if(field==NULL || value==NULL) return;

	if(!strcmp(field,"UnixPath")) 				strncpy(conf->UnixPath,value,strlen(value));
	else if(!strcmp(field,"MaxConnections")) 	conf->MaxConnections=(unsigned long)atoi(value);
	else if(!strcmp(field,"ThreadsInPool"))  	conf->ThreadsInPool=(unsigned long)atoi(value);
	else if(!strcmp(field,"MaxMsgSize"))	 	conf->MaxMsgSize=(unsigned long)atoi(value);
	else if(!strcmp(field,"MaxFileSize"))	 	conf->MaxFileSize=(unsigned long)atoi(value);
	else if(!strcmp(field,"MaxHistMsgs"))	 	conf->MaxHistMsgs=(unsigned long)atoi(value);
	else if(!strcmp(field,"DirName")) 			strncpy(conf->DirName,value,strlen(value));
	else if(!strcmp(field,"StatFileName")) 		strncpy(conf->StatFileName,value,strlen(value));
}

void set_configuration(struct Config* conf, FILE* s){

	
	char buf[200];
	char *delim="	\n ";
	char *field;
	char *value;
	//char *ptr;
	char *c;
	c=fgets(buf,200,s); 
	while(c!=NULL){						  			//leggo una riga alla volta
		
		field=strtok/*_r*/(buf,delim/*,&ptr*/);				//tokenizzo la prima parola
		value=strtok/*_r*/(NULL,delim/*,&ptr*/);			//tolgo l'=
		value=strtok/*_r*/(NULL,delim/*,&ptr*/);			//tokenizzo il valore da inserire
		
		put(conf,field,value);
		c=fgets(buf,200,s);
	}

}
