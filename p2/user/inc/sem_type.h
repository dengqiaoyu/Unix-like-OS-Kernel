/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <mutex.h>
#include <cond.h>

typedef struct sem {
    int count;
    int is_act;
    mutex_t mutex;
    cond_t cond;
} sem_t;

#endif /* _SEM_TYPE_H */
