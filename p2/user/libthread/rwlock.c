#include <rwlock.h>
#include <sem.h>
#include <assert.h>
#include "rwlock_type.h"


int rwlock_init(rwlock_t *rwlock) {
    assert(rwlock);
    rwlock->reader_in_cri_sec = 0;
    rwlock->reader_out_cri_sec = 0;
    if (sem_init(&(rwlock->in_sem), 1)) return ERROR_RWLOCK_INIT_FAILED;
    if (sem_init(&(rwlock->out_sem), 1)) return ERROR_RWLOCK_INIT_FAILED;
    if (sem_init(&(rwlock->writer_sem), 0)) return ERROR_RWLOCK_INIT_FAILED;
    rwlock->is_writer_waiting = 0;
    rwlock->is_act = 1;
    return SUCCESS;
}

void rwlock_destroy(rwlock_t *rwlock) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    sem_destroy(&(rwlock->in_sem));
    sem_destroy(&(rwlock->out_sem));
    sem_destroy(&(rwlock->writer_sem));
    rwlock->is_writer_waiting = 0;
    rwlock->is_act = 0;
}

void rwlock_lock(rwlock_t *rwlock, int type) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    if (rwlock->is_act == 0) return;
    if (type == RWLOCK_READ) {
        sem_wait(&(rwlock->in_sem));
        rwlock->reader_in_cri_sec++;
        sem_signal(&(rwlock->in_sem));
    } else {
        sem_wait(&(rwlock->in_sem));
        sem_wait(&(rwlock->out_sem));
        if (rwlock->reader_in_cri_sec == rwlock->reader_out_cri_sec) {
            sem_signal(&(rwlock->out_sem));
        } else {
            rwlock->is_writer_waiting = 1;
            sem_signal(&(rwlock->out_sem));
            sem_wait(&(rwlock->writer_sem));
            rwlock->is_writer_waiting = 0;
        }
    }
}

void rwlock_unlock(rwlock_t *rwlock) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    if (rwlock->is_act == 0) return;
    if (rwlock->reader_in_cri_sec != rwlock->reader_out_cri_sec) {
        sem_wait(&(rwlock->out_sem));
        rwlock->reader_out_cri_sec++;
        if (rwlock->is_writer_waiting == 1
                && rwlock->reader_in_cri_sec == rwlock->reader_out_cri_sec) {
            sem_signal(&(rwlock->writer_sem));
        }
        sem_signal(&(rwlock->out_sem));
    } else {
        sem_signal(&(rwlock->in_sem));
    }
}

void rwlock_downgrade(rwlock_t *rwlock) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    if (rwlock->is_act == 0) return;
    if (rwlock->reader_in_cri_sec != rwlock->reader_out_cri_sec) return;
    rwlock->reader_in_cri_sec++;
    sem_signal(&(rwlock->in_sem));
}
