/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <sem.h>
#include "return_type.h"

typedef struct rwlock {
    unsigned int reader_in_cri_sec;
    unsigned int reader_out_cri_sec;
    sem_t in_sem;
    sem_t out_sem;
    sem_t writer_sem;
    int is_writer_waiting;
    int is_act;
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
