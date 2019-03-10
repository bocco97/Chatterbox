/**
 * @file
 *
 * Header file for icl_hash routines.
 *
 */
/* $Id$ */
/* $UTK_Copyright: $ */

#ifndef icl_hash_h
#define icl_hash_h

#include <stdio.h>

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>


#include<config.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif






typedef struct icl_entry_s {
    void* key;
    void *data;
    int histSize;
    struct icl_entry_s* next;
} icl_entry_t;

typedef struct icl_hash_s {
    int nbuckets;
    int nentries;
    icl_entry_t **buckets;
    pthread_mutex_t qlock [N_SECTIONS];
    unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);
} icl_hash_t;


void LockHash(pthread_mutex_t *qlock, int i);

void UnlockHash(pthread_mutex_t *qlock, int i);

icl_hash_t *
icl_hash_create(unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) );

icl_entry_t
* icl_hash_find(icl_hash_t *, void* );

icl_entry_t
* icl_hash_insert(icl_hash_t *, void*, void *);

int
icl_hash_destroy(icl_hash_t *, void (*)(void*), void (*)(void*)),
    icl_hash_dump(FILE *, icl_hash_t *);

int icl_hash_delete( icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*) );


#define icl_hash_foreach(ht, tmpint, tmpent, kp, dp)    \
    for (tmpint=0;tmpint<ht->nbuckets; tmpint++)        \
        for (tmpent=ht->buckets[tmpint];                                \
             tmpent!=NULL&&((kp=tmpent->key)!=NULL)&&((dp=tmpent->data)!=NULL); \
             tmpent=tmpent->next)


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* icl_hash_h */
