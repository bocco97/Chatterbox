/** \file stats.h  
    \author Lorenzo Boccolini 544098
    Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
    originale dell'autore  
*/

/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>
#include <pthread.h>

struct statistics {
    unsigned long nusers;                       // n. di utenti registrati
    unsigned long nonline;                      // n. di utenti connessi
    unsigned long ndelivered;                   // n. di messaggi testuali consegnati
    unsigned long nnotdelivered;                // n. di messaggi testuali non ancora consegnati
    unsigned long nfiledelivered;               // n. di file consegnati
    unsigned long nfilenotdelivered;            // n. di file non ancora consegnati
    unsigned long nerrors;                      // n. di messaggi di errore
};

/* aggiungere qui altre funzioni di utilita' per le statistiche */

static inline void ADD_ERROR(pthread_mutex_t *lock, struct statistics * stat){
    pthread_mutex_lock(&lock[6]);
    stat->nerrors +=1;
    pthread_mutex_unlock(&lock[6]);
}



static inline void ADD_USER(pthread_mutex_t *lock, struct statistics * stat){
    pthread_mutex_lock(&lock[0]);
    stat->nusers =(int)stat->nusers+1;
    pthread_mutex_unlock(&lock[0]);
}


static inline void DEL_USER(pthread_mutex_t *lock, struct statistics * stat){
    pthread_mutex_lock(&lock[0]);
    if((int)stat->nusers>=1) stat->nusers =(int)stat->nusers-1;
    pthread_mutex_unlock(&lock[0]);
}


static inline void ADD_MSG_DEL(pthread_mutex_t *lock, struct statistics * stat){
    pthread_mutex_lock(&lock[2]);
    stat->ndelivered +=1;
    pthread_mutex_unlock(&lock[2]);
}


static inline void ADD_MSG_UNDEL(pthread_mutex_t *lock, struct statistics * stat){
    pthread_mutex_lock(&lock[3]);
    stat->nnotdelivered +=1;
    pthread_mutex_unlock(&lock[3]);
}


static inline void ADD_FILE_DEL(pthread_mutex_t *lock, struct statistics * stat){
    pthread_mutex_lock(&lock[4]);
    stat->nfiledelivered +=1;
    pthread_mutex_unlock(&lock[4]);
}

static inline void ADD_FILE_UNDEL(pthread_mutex_t *lock, struct statistics * stat){
    pthread_mutex_lock(&lock[5]);
    stat->nfilenotdelivered +=1;
    pthread_mutex_unlock(&lock[5]);
}

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento 
 */
static inline int printStats(FILE *fout) {
    extern struct statistics chattyStats;

    if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
		(unsigned long)time(NULL),
		chattyStats.nusers, 
		chattyStats.nonline,
		chattyStats.ndelivered,
		chattyStats.nnotdelivered,
		chattyStats.nfiledelivered,
		chattyStats.nfilenotdelivered,
		chattyStats.nerrors
		) < 0) return -1;
    fflush(fout);
    return 0;
}

#endif /* MEMBOX_STATS_ */
