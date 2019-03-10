#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <queue.h>
#include <message.h>
#include<stdio.h>
/**
 * @file queue.c
 * @brief File di implementazione dell'interfaccia per la coda
 */





/* ------------------- funzioni di utilita' -------------------- */

static Node_t *allocNode()         { return (Node_t*)malloc(sizeof(Node_t));  }
static Queue_t *allocQueue()       { return (Queue_t*)malloc(sizeof(Queue_t)); }
static void freeNode(Node_t *node) { if(node) free((void*)node); }
static void LockQueue(Queue_t * q)            { pthread_mutex_lock(&q->qlock); }
static void UnlockQueue(Queue_t * q)          { pthread_mutex_unlock(&q->qlock); }
static void UnlockQueueAndWait(Queue_t * q)   { pthread_cond_wait(&q->qcond, &q->qlock); }
static void UnlockQueueAndSignal(Queue_t * q) {
    pthread_cond_signal(&q->qcond);
    pthread_mutex_unlock(&q->qlock);
}
/* ------------------- interfaccia della coda ------------------ */

Queue_t *initQueue() {
    Queue_t *q = allocQueue();
    if (!q) return NULL;
    memset(q,0,sizeof(*q));                        
    q->head = NULL;
    q->tail = q->head;    
    q->qlen = 0;
    pthread_mutex_init(&q->qlock,NULL);
    pthread_cond_init(&q->qcond,NULL);
    return q;
}

void delete_msg_Queue(Queue_t *q) {
    while(q->head != NULL) {
	Node_t *p = (Node_t*)q->head;
	q->head = q->head->next;
    message_t *tmp=NULL;
    if(p && p->data) tmp=(message_t*)p->data;
    if(tmp && tmp->data.buf) {
        if(tmp->data.buf) free((char*)tmp->data.buf);
        if(tmp) free((message_t*)tmp);
    }
	if(p) freeNode(p);
    }
    if(q) free(q); 
}


void deleteQueue(Queue_t *q){
    while(q->head != NULL) {
    Node_t *p = (Node_t*)q->head;
    q->head = q->head->next;
    freeNode(p);
    }
    if (q->head) freeNode((void*)q->head);
    if(q)free(q);
}


int push(Queue_t *q, void *data) {
    Node_t *n = allocNode();
    if(n) memset(n,0,sizeof(*n));                               
    n->data = data; 
    n->next = NULL;
    LockQueue(q);
    if(q->head==NULL ) {
        q->head=n;
        q->tail=n;
    }
    else{
        q->tail->next = n;
        q->tail       = n;
    }
    q->qlen      += 1;
    UnlockQueueAndSignal(q);
    return 0;
}

void *pop(Queue_t *q) {        
    LockQueue(q);
    while(q->head == NULL) {
	UnlockQueueAndWait(q);
    }
    // locked
    assert(q->head);
    Node_t *n  = (Node_t *)q->head;  
    void *data = (q->head)->data;
    if(q->tail==q->head) q->tail=NULL;
    q->head    = q->head->next;
    q->qlen   -= 1;
    assert(q->qlen>=0);
    if(n) free(n);
    UnlockQueue(q);
    return data;
} 


int length(Queue_t *q) {
    LockQueue(q);
    int len = q->qlen;
    UnlockQueue(q);
    return len;
}