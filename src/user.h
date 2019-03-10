/** \file user.h  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/

#include<config.h>
#include <pthread.h>
#include<message.h>

typedef struct user{

	char nickname[MAX_NAME_LENGTH+1];
	int fd;
	int valid;

}	User;


typedef struct online{

	User *users;
	int max_online;
	int	users_online;
	pthread_mutex_t lock;

}	online_set;


/* 
Inserisce un utente di nickname=name con file_descriptor=fd 
ritorna 0 in caso di successo(connessione) o -1 in caso di errore(utente già connesso o max_utenti già connessi)
*/
int connect_user(online_set* set,int fd, char* name);

/*
Disconnette un utente di nickname=name, se questo è presente in set
*/
void disconnect_user(online_set* set, char* name);


/*
Disconnette un utente associato al fd
*/
void disconnect_by_fd(online_set* set,int fd);


/*
verifica se l'utente di nome=name risulta connesso in set
ritorna 1 se l'utente è connesso, 0 altrimenti
*/
int is_online(online_set* set, char* name);

/*
cerca l'utente di nickname name nell'online_set set
restituisce il fd associato, se viene trovato, -1 altrimenti
*/
int get_fd(online_set* set, char* name);

/*
inizializza l'online_set set, settando max_users come numero massimo di utenti connessi
ritorna -1 in caso di errore, 0 altrimenti
*/
int initialize_online_set(online_set* set, int max_users);

/*
libera la memoria occupata dall' online_set puntato da set
*/
void delete_online_set(online_set* set);