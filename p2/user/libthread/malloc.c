/** @file malloc.c
 *  @brief Implements thread safe malloc().
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>

#include <mutex.h>

mutex_t mutex = INIT_MUTEX;

void *malloc(size_t __size) {
    mutex_lock(&mutex);
    void *ret = _malloc(__size);
    mutex_unlock(&mutex);
    return ret;
}

void *calloc(size_t __nelt, size_t __eltsize) {
    mutex_lock(&mutex);
    void *ret = _calloc(__nelt, __eltsize);
    mutex_unlock(&mutex);
    return ret;
}

void *realloc(void *__buf, size_t __new_size) {
    mutex_lock(&mutex);
    void *ret = _realloc(__buf, __new_size);
    mutex_unlock(&mutex);
    return ret;
}

void free(void *__buf) {
    mutex_lock(&mutex);
    _free(__buf);
    mutex_unlock(&mutex);
    return;
}

