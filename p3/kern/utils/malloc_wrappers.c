#include <stddef.h>
#include <malloc.h>
#include <malloc_internal.h>
#include "utils/kern_mutex.h"

// allows use of malloc before malloc_mutex can be initialized
// necessary because kern_mutex_init itself calls malloc
kern_mutex_t malloc_mutex = {.is_locked = 0};

/* safe versions of malloc functions */
void *malloc(size_t size) {
    kern_mutex_lock(&malloc_mutex);
    void *ret =  _malloc(size);
    kern_mutex_unlock(&malloc_mutex);
    return ret;
}

void *memalign(size_t alignment, size_t size) {
    kern_mutex_lock(&malloc_mutex);
    void *ret =  _memalign(alignment, size);
    kern_mutex_unlock(&malloc_mutex);
    return ret;
}

void *calloc(size_t nelt, size_t eltsize) {
    kern_mutex_lock(&malloc_mutex);
    void *ret =  _calloc(nelt, eltsize);
    kern_mutex_unlock(&malloc_mutex);
    return ret;
}

void *realloc(void *buf, size_t new_size) {
    kern_mutex_lock(&malloc_mutex);
    void *ret =  _realloc(buf, new_size);
    kern_mutex_unlock(&malloc_mutex);
    return ret;
}

void free(void *buf) {
    kern_mutex_lock(&malloc_mutex);
    _free(buf);
    kern_mutex_unlock(&malloc_mutex);
}

void *smalloc(size_t size) {
    kern_mutex_lock(&malloc_mutex);
    void *ret =  _smalloc(size);
    kern_mutex_unlock(&malloc_mutex);
    return ret;
}

void *smemalign(size_t alignment, size_t size) {
    kern_mutex_lock(&malloc_mutex);
    void *ret =  _smemalign(alignment, size);
    kern_mutex_unlock(&malloc_mutex);
    return ret;
}

void sfree(void *buf, size_t size) {
    kern_mutex_lock(&malloc_mutex);
    _sfree(buf, size);
    kern_mutex_unlock(&malloc_mutex);
}


