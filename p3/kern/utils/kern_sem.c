#include <stdlib.h>
#include <mutex.h>
#include <assert.h>
#include "kern_sem.h"

int kern_sem_init(kern_sem_t *sem, int count) {
    assert(sem != NULL);
    int ret = kern_cond_init(&sem->cond);
    assert(ret == 0);
    ret = mutex_init(&sem->mutex);
    assert(ret == 0);
    sem->is_active = 1;
    sem->count = count;
    return 0;
}

void kern_sem_destroy(kern_sem_t *sem) {
    assert(sem != NULL);
    assert(sem->is_active == 1);
    mutex_destroy(&sem->mutex);
    kern_cond_destroy(&sem->cond);
    sem->is_active = 0;
}

void kern_sem_wait(kern_sem_t *sem) {
    assert(sem != NULL);
    assert(sem->is_active == 1);
    mutex_lock(&sem->mutex);
    while (sem->count == 0) {
        kern_cond_wait(&sem->cond, &sem->mutex);
    }
    sem->count--;
    mutex_unlock(&sem->mutex);
}

void kern_sem_signal(kern_sem_t *sem) {
    assert(sem != NULL);
    assert(sem->is_active == 1);
    mutex_lock(&sem->mutex);
    sem->count++;
    kern_cond_signal(&sem->cond);
    mutex_unlock(&sem->mutex);
}
