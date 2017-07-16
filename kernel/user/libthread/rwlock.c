/**
 * @file rwlock.c
 * @brief This file contains basic function operations for reader and writer
 *        lock such as initialize, destroy, lock, unlockn and downgrade.
 * This implementation uses three different semaphores as a FIFO likely queue:
 * in_sem: This is the overall queue for all of thread that want to get lock,
 *         what's more, it protects the "reader_in_cri_sec" which means how many
 *         readers has get into the critical section, so this ensures that
 *         writers and reader in "queue" will pass insem in the order of they
 *         arrived, which avoid the starvation of any kind of threads.
 * out_sem: This is just used as a queued mutex for protecting
 *          "reader_out_cri_sec" which means the number of readers that have
 *          exit the critical section, once we can protect this varibale by
 *          preventing two readers change it at the same time, we can compare it
 *          with "reader_in_cri_sec", if they are equal we can know that there
 *          are readers in the critical section, othheerwise no readers which
 *          can be used by writer to check whether they can get the lock.
 * writer_sem: this is a exclusive mutex that is used for writer to write. But
 *             it is will not be relased by the writer itself, instead reader
 *             is responseable for signal that semaphore. So Any writer that is
 *             waiting on writer_sem should always wait the next reader to give
 *             the write right to it, thus we can avoid reader starvation which
 *             can be seen at second reader and writer problem. And only when
 *             there are readers in the criticalk section the writer will wait
 *             on this semaphore.
 * This implementation can prevent starvation both for writer and reader,
 * first because it uses a FIFO queue, the only competition is only when they
 * are added into queue, since mutex is random scheduled to every thread that
 * requires it, we can say it is fair. Second, if there is no writer in front of
 * a reader, it can enter the cirtical section immediatelly and they do not need
 * to hold any mutex, which is very efficient. What's more, since no writer is
 * allowed to release the writer_sem, so we always need readers to give that sem
 * to writer, it can prevent reader's starvation.
 * For downgrade part, the writer just need to increase the reader_in_cri_sec,
 * and then signal in_sem since a writer will always hold to prevent any reader
 * get into critical section.
 *
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <rwlock.h>
#include <sem.h>    /* sem_t, sem_init, sem_destroy, sem_wait, sem_signal */
#include <assert.h> /* assert */
#include "rwlock_type.h" /* rwlock_t, return value*/

/**
 * Initialize the reader and writer lock, the pointer of rwlock should not be
 * NULL.
 * @param  rwlock the pointer to reader and writer lock.
 * @return        SUCCESS(0) as success, ERROR_RWLOCK_INIT_FAILED(-1) as fail.
 */
int rwlock_init(rwlock_t *rwlock) {
    assert(rwlock);
    rwlock->reader_in_cri_sec = 0;
    rwlock->reader_out_cri_sec = 0;
    if (sem_init(&(rwlock->in_sem), 1)) return ERROR_RWLOCK_INIT_FAILED;
    if (sem_init(&(rwlock->out_sem), 1)) return ERROR_RWLOCK_INIT_FAILED;
    /* at first writer is not allowed to get lock, unless reader give it. */
    if (sem_init(&(rwlock->writer_sem), 0)) return ERROR_RWLOCK_INIT_FAILED;
    rwlock->is_writer_waiting = 0;
    rwlock->is_act = 1;
    return SUCCESS;
}

/**
 * Destroy and deactivate the reader and writer lock, crash when destroy a
 * rwlock that is under used.
 * @param rwlock the pointer to reader and writer lock
 */
void rwlock_destroy(rwlock_t *rwlock) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    sem_destroy(&(rwlock->in_sem));
    sem_destroy(&(rwlock->out_sem));
    sem_destroy(&(rwlock->writer_sem));
    rwlock->is_writer_waiting = 0;
    rwlock->is_act = 0;
}

/**
 * Lock the rwlock according the "type" pointer, reader lock as RWLOCK_READ
 * writer lock as RWLOCK_WRITE
 * @param rwlock the pointer to reader and writer lock
 * @param type   reader or writer lock(RWLOCK_READ, RWLOCK_WRITE)
 */
void rwlock_lock(rwlock_t *rwlock, int type) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    if (type == RWLOCK_READ) {
        /*
         * get into queue to modify the number of readers that is on critical
         * section.
         */
        sem_wait(&(rwlock->in_sem));
        rwlock->reader_in_cri_sec++;
        sem_signal(&(rwlock->in_sem));
    } else {
        /* get into queue in order */
        sem_wait(&(rwlock->in_sem));
        /*
         * to ensure that there is no reader is changing the number of reader
         * has exited critical section.
         */
        sem_wait(&(rwlock->out_sem));
        if (rwlock->reader_in_cri_sec == rwlock->reader_out_cri_sec) {
            sem_signal(&(rwlock->out_sem));
        } else {
            /*
             * this line is intended put in front of signal because we need to
             * prevent child check is_writer_waiting before we change it.
             */
            rwlock->is_writer_waiting = 1;
            sem_signal(&(rwlock->out_sem));
            /* require the exclusive mutex(or semaphore) */
            sem_wait(&(rwlock->writer_sem));
            /* clear the flag*/
            rwlock->is_writer_waiting = 0;
        }
    }
}

/**
 * Unlock the rwlock now matter what types of locks the thread is holding. On
 * the calling of this function, other thread will be waked by the caller.
 * @param rwlock the pointer to reader and writer lock
 */
void rwlock_unlock(rwlock_t *rwlock) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    /*
     * Here we do not use out_sem to protect "reader_out_cri_sec" because if
     * the thread that is unlock rwlock is a writer then there will be no reader
     * that can change reader_out_cri_sec, since they all block at
     * sem_wait(&(rwlock->in_sem)) because wirter needs to hold "in_sem"
     * all the time, if the thread is a reader, reader_out_cri_sec can be
     * changed at the same time but since thread itself is a reader,
     * "reader_out_cri_sec" can never be equal to "reader_in_cri_sec".
     */
    if (rwlock->reader_in_cri_sec != rwlock->reader_out_cri_sec) {
        /* get rwlock->out_sem to increament "reader_out_cri_sec" */
        sem_wait(&(rwlock->out_sem));
        rwlock->reader_out_cri_sec++;
        /*
         * If there is no other reader in critical section and a wrtier is
         * waiting in writer_sem, reader which is the last one exiting critical
         * section need to give that lock to the writer.
         */
        if (rwlock->is_writer_waiting == 1
                && rwlock->reader_in_cri_sec == rwlock->reader_out_cri_sec) {
            sem_signal(&(rwlock->writer_sem));
        }
        /*
         * To prevent writer change is_writer_waiting, when thread is accessing
         * it.
         */
        sem_signal(&(rwlock->out_sem));
    } else {
        /*
         * Release the "queue" semaphore to let next reader or writer to get in
         */
        sem_signal(&(rwlock->in_sem));
    }
}

/**
 * Downgrade the writer lock to reader lock, when writer calls this function,
 * other function that is waiting a reader lock will immediately get the lock.
 * @param rwlock the pointer to reader and writer lock
 */
void rwlock_downgrade(rwlock_t *rwlock) {
    assert(rwlock);
    assert(rwlock->is_act == 1);
    if (rwlock->reader_in_cri_sec != rwlock->reader_out_cri_sec) return;
    rwlock->reader_in_cri_sec++;
    sem_signal(&(rwlock->in_sem));
}
