/**
 * @file sem.c
 * @brief semaphore implementation
 * @author Newton Xie(ncx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <syscall.h>
#include <malloc.h>
#include <assert.h>
#include <simics.h>
#include <stdio.h>
#include <sem_type.h>

int sem_init(sem_t *sem, int count) {
    if (mutex_init(&(sem->mutex)) < 0) return -1;
    if (cond_init(&(sem->cond)) < 0) {
        mutex_destroy(&(sem->mutex));
        return -1;
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

