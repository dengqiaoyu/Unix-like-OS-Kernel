/** @file mutex.h
 *  @brief Mutex header file.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <stdint.h>
#include "utils/list.h"

typedef struct mutex {
    int is_locked;
    void *mutex_holder;
    list_t *blocked_list;
} kern_mutex_t;

int kern_mutex_init(kern_mutex_t *mp);

void kern_mutex_destroy(kern_mutex_t *mp);

void kern_mutex_lock(kern_mutex_t *mp);

void kern_mutex_unlock(kern_mutex_t *mp);

void cli_kern_mutex_unlock(kern_mutex_t *mp);

#endif
