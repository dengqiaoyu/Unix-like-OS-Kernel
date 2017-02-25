/** @file sem.c
 *  @brief Implements semaphores.
 *
 *  Uses a straightforward implementation of semaphores with a mutex,
 *  a conditional variable, and a count variable. The mutex protects
 *  the count variable from unsafe accesses. The sem_wait() function
 *  simply decrements the count variable, blocking on the conditional
 *  variable first if necessary. The sem_signal() function increments
 *  the count variable and attempts to wake a waiter if the count was
 *  incremented from zero to one.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#include <syscall.h>
#include <malloc.h>
#include <assert.h>
#include <simics.h>
#include <stdio.h>
#include <sem_type.h>

int sem_init(sem_t *sem, int count) {
    if (mutex_init(&(sem->mutex)) < 0) return ERROR_SEM_INIT_FAILED;
    if (cond_init(&(sem->cond)) < 0) {
        mutex_destroy(&(sem->mutex));
        return ERROR_SEM_INIT_FAILED;
    }
    sem->count = count;
    sem->is_act = 1;
    return SUCCESS;
}

void sem_destroy(sem_t *sem) {
    assert(sem != NULL);
    assert(sem->is_act == 1);
    mutex_destroy(&(sem->mutex));
    cond_destroy(&(sem->cond));
    sem->count = 0;
    sem->is_act = 0;
}

void sem_wait(sem_t *sem, mutex_t *mp) {
    assert(sem != NULL);
    assert(sem->is_act == 1);
    mutex_lock(&(sem->mutex));
    while (sem->count == 0)
        cond_wait(&(sem->cond), &(sem->mutex));
    sem->count--;
    mutex_unlock(&(sem->mutex));
}

void sem_signal(sem_t *sem) {
    assert(sem != NULL);
    assert(sem->is_act == 1);
    mutex_lock(&(sem->mutex));
    int count = sem->count++;
    mutex_unlock(&(sem->mutex));
    if (count == 0)
        cond_signal(&(sem->cond));
}

