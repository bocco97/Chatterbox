/** \file dataHandler.h  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/

#include<icl_hash.h>
#include<message.h>


/*
Tenta di aggiungere un messaggio msg nella coda di dest, eliminando il primo messaggio dalla coda se questa ha una lunghezza >= max
ritorna -1 in caso di errore, 0 altrimenti
*/
int add_msg(icl_hash_t *ht,icl_entry_t *dest, message_t* msg, int max);

/*
Cerca un file di nome filename nella coda dell'icl_entry_t getter:
Ritorna 1 in caso di successo, 0 altrimenti
*/
int is_in_history(icl_hash_t* ht, icl_entry_t* getter, char* filename);