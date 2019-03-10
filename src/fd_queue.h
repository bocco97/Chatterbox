/** \file fd_queue.h  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/
#ifndef FD_QUEUE_H_
#define FD_QUEUE_H_
#include <pthread.h>
/** Elemento della coda
 */
typedef struct fdnode {
   	int        	fd;
    struct fdnode * next;
} node;

/** Struttura dati coda
 */
typedef struct fdQueue {
    node        *head;
    node        *tail;
    int qlen;
    pthread_mutex_t qlock;
    pthread_cond_t  qcond;
} fd_queue;


/** Alloca una coda di tipo fd_queue.
 *   ritorna NULL se si sono verificati problemi nell'allocazione (errno settato),
 *   altrimenti q puntatore alla coda allocata
 */
fd_queue *init_fd_queue();

/** Cancella una coda allocata con init_fd_queue.
 *   \q puntatore alla coda da cancellare
 */
void delete_fd_queue(fd_queue *q);

/** Inserisce un intero nella coda.
 *   \fd intero da inserire
 */
void   fd_push(fd_queue *q, int fd);

/** Estrae un intero dalla coda.
 *  \ritorna intero estratto da q
 */
int  fd_pop(fd_queue *q);

/** Restituisce la lunghezza della coda
	\q puntatore alla coda 
 */

int fdq_length(fd_queue *q);

#endif