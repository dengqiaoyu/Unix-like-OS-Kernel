#include <stddef.h>
#include <malloc.h>
#include <malloc_internal.h>
#include "mutex.h"

mutex_t mutex = INIT_MUTEX;

/* safe versions of malloc functions */
void *malloc(size_t size)
{
    mutex_lock(&mutex);
    void *ret =  _malloc(size);
    mutex_unlock(&mutex);
    return ret;
}

void *memalign(size_t alignment, size_t size)
{
    mutex_lock(&mutex);
    void *ret =  _memalign(alignment, size);
    mutex_unlock(&mutex);
    return ret;
}

void *calloc(size_t nelt, size_t eltsize)
{
    mutex_lock(&mutex);
    void *ret =  _calloc(nelt, eltsize);
    mutex_unlock(&mutex);
    return ret;
}

void *realloc(void *buf, size_t new_size)
{
    mutex_lock(&mutex);
    void *ret =  _realloc(buf, new_size);
    mutex_unlock(&mutex);
    return ret;
}

void free(void *buf)
{
    mutex_lock(&mutex);
    _free(buf);
    mutex_unlock(&mutex);
}

void *smalloc(size_t size)
{
    mutex_lock(&mutex);
    void *ret =  _smalloc(size);
    mutex_unlock(&mutex);
    return ret;
}

void *smemalign(size_t alignment, size_t size)
{
    mutex_lock(&mutex);
    void *ret =  _smemalign(alignment, size);
    mutex_unlock(&mutex);
    return ret;
}

void sfree(void *buf, size_t size)
{
    mutex_lock(&mutex);
    _sfree(buf, size);
    mutex_unlock(&mutex);
}


