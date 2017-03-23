/** @file mutex.c
 *  @brief Implements mutexes.
 *
 *  Uses a very simple spin waiting implementation.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#include <mutex.h>

/* declare assembly helpers */
int mutex_lock_asm(mutex_t *mp);
int mutex_unlock_asm(mutex_t *mp);

int mutex_init(mutex_t *mp) {
    mp->lock = 0;
    return 0;
}

void mutex_destroy(mutex_t *mp) {
    return;
}

void mutex_lock(mutex_t *mp) {
    int ret;
    do {
        ret = mutex_lock_asm(mp);
    } while (ret != 0);
}

void mutex_unlock(mutex_t *mp) {
    mutex_unlock_asm(mp);
}

