/** @file mutex.h
 *  @brief Mutex hader file.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#ifndef _MUTEX_H_
#define _MUTEX_H_

#define INIT_MUTEX {.lock = 0}

typedef struct mutex {
    int lock;
} mutex_t;

int mutex_init(mutex_t *mp);

void mutex_destroy(mutex_t *mp);

void mutex_lock(mutex_t *mp);

void mutex_unlock(mutex_t *mp);

#endif
