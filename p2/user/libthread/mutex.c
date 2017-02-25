/** @file mutex.c
 *  @brief Implements mutexes.
 *
 *  Uses a very simple spin waiting implementation. The mutex has a
 *  holder_tid attribute, and a thread which successfully acquires
 *  the mutex will update this field. Subsequent mutex_lock() callers
 *  will attempt to yield to the holding thread. The holder_tid is
 *  restored to -1 before unlocking.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#include <mutex.h>
#include <syscall.h>

/* declare assembly helpers */
int mutex_lock_asm(mutex_t *mp, int tid);
int mutex_unlock_asm(mutex_t *mp);

int mutex_init(mutex_t *mp) {
    mp->lock = 0;
    mp->holder_tid = -1;
    return SUCCESS;
}

void mutex_destroy(mutex_t *mp) {
    return;
}

void mutex_lock(mutex_t *mp) {
    int tid = gettid();
    int ret = mutex_lock_asm(mp, tid);
    while (ret != 0) {
        yield(mp->holder_tid);
        ret = mutex_lock_asm(mp, tid);
    }
}

void mutex_unlock(mutex_t *mp) {
    mp->holder_tid = -1;
    mutex_unlock_asm(mp);
}

