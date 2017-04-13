#ifndef _KERN_SEM_H_
#define _KERN_SEM_H_

#include <mutex.h>
#include "kern_cond.h"

typedef struct kern_sem {
    char is_active;
    int count;
    mutex_t mutex;
    kern_cond_t cond;
} kern_sem_t;

int kern_sem_init(kern_sem_t *sem, int count);

void kern_sem_destroy(kern_sem_t *sem);

void sem_wait(kern_sem_t *sem);

void sem_signal(kern_sem_t *sem);

#endif