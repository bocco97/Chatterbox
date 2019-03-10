/** \file fd_queue.c
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <fd_queue.h>
#include <stdio.h>
#include <string.h>




/* ------------------- funzioni di utilita' -------------------- */

static node *alloc_node()                          { return (node*)malloc(sizeof(node));  }
static fd_queue *alloc_fd_queue()                  { return (fd_queue*)malloc(sizeof(fd_queue)); }
static void free_node(node *n)                     { if(n) free(n); }
static void Lock_fd_queue(fd_queue * q)            { pthread_mutex_lock(&q->qlock); }
static void Unlock_fd_queue(fd_queue * q)          { pthread_mutex_unlock(&q->qlock); }
static void Unlock_fd_queueAndWait(fd_queue * q)   { pthread_cond_wait(&q->qcond, &q->qlock); }
static void Unlock_fd_queueAndSignal(fd_queue * q) {
    pthread_cond_signal(&q->qcond);
    pthread_mutex_unlock(&q->qlock);
}
/* ------------------- interfaccia della coda ------------------ */

fd_queue *init_fd_queue() {
    fd_queue *q = alloc_fd_queue();
    if (!q) return NULL;
    memset(q,0,sizeof(*q));                            
    q->head = NULL;
    q->tail = q->head;    
    q->qlen = 0;
    pthread_mutex_init(&q->qlock,NULL);
    pthread_cond_init(&q->qcond,NULL);
    return q;
}


void delete_fd_queue(fd_queue *q){
    while(q->head != NULL) {
    node *p = (node*)q->head;
    q->head = q->head->next;
    free_node(p);
    }
    if (q->head) free_node((void*)q->head);
    if(q)free(q);
}


void fd_push(fd_queue *q, int fdes) {
    node *n = alloc_node();
    if(n) memset(n,0,sizeof(*n));                               
    n->fd = fdes; 
    n->next = NULL;
    Lock_fd_queue(q);
    if(q->head==NULL ) {
        q->head=n;
        q->tail=n;
    }
    else{
        q->tail->next = n;
        q->tail       = n;
    }
    q->qlen      += 1;
    Unlock_fd_queueAndSignal(q);
}


int fd_pop(fd_queue *q) {        
    Lock_fd_queue(q);
    while(q->head == NULL) {
	Unlock_fd_queueAndWait(q);
    }
    // locked
    assert(q->head);
    node *n  = (node *)q->head;  
    int data = (q->head)->fd;
    if(q->tail==q->head) q->tail=NULL;
    q->head    = q->head->next;
    q->qlen   -= 1;
    assert(q->qlen>=0);
    if(n) free(n);
    Unlock_fd_queue(q);
    
    return data;
} 


int fdq_length(fd_queue *q) {
    Lock_fd_queue(q);
    int len = q->qlen;
    Unlock_fd_queue(q);
    return len;
}