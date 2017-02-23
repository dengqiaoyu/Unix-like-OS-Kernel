/** @file mutex.c
 *  @brief
 */

#include <mutex.h>
#include <syscall.h>

int mutex_lock_asm(mutex_t *mp, int tid);
int mutex_unlock_asm(mutex_t *mp);

int mutex_init(mutex_t *mp) {
  mp->lock = 0;
  mp->holder_tid = -1;
  return 0;
}

void mutex_destroy(mutex_t *mp) {
  return;
}

void mutex_lock(mutex_t *mp) {
    int tid = gettid();
    int ret = mutex_lock_asm(mp, tid);
    /*
    int yield_flag = 1;
    while (ret != 0) {
        if (yield_flag) {
            if (mp->holder_tid != -1) {
                if (yield(mp->holder_tid) < 0) yield_flag = 0;
            }
            else {
                if (yield(-1) < 0) yield_flag = 0;
            }
        }
        ret = mutex_lock_asm(mp, tid);
    }
    */
    while (ret != 0) {
        if (mp->holder_tid != -1) {
            yield(mp->holder_tid);
            ret = mutex_lock_asm(mp, tid);
        }
        else {
            yield(-1);
            ret = mutex_lock_asm(mp, tid);
        }
    }
}

void mutex_unlock(mutex_t *mp) {
    mp->holder_tid = -1;
    mutex_unlock_asm(mp);
}

