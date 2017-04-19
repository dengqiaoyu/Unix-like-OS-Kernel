/**
 * @file rwlock_type.h
 * @brief This file defines the type for reader/writer locks.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <sem.h> /* sem_t, sem_init, sem_destroy, sem_wait, sem_signal */
#include "return_type.h"

typedef struct rwlock {
    /* the number of readers that has enter critical section */
    unsigned int reader_in_cri_sec;
    /* the number of readers that has exit critical section */
    unsigned int reader_out_cri_sec;
    /* see rwlock.c for details */
    sem_t in_sem;
    sem_t out_sem;
    sem_t writer_sem;
    /* indicate if there is a writer waiting in the queue */
    int is_writer_waiting;
    /* whther this rwlock is activated */
    int is_act;
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
