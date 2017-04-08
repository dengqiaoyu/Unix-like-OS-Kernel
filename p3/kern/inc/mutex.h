/** @file mutex.h
 *  @brief Mutex header file.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "list.h"

typedef struct mutex {
    int lock;
    list_t list;
} mutex_t;

int mutex_init(mutex_t *mp);

void mutex_destroy(mutex_t *mp);

void mutex_lock(mutex_t *mp);

void mutex_unlock(mutex_t *mp);

#endif
