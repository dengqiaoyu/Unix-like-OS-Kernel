#include <stddef.h>
#include <malloc.h>
#include <malloc_internal.h>
#include "mutex.h"

// allows use of malloc before malloc_mutex can be initialized
// necessary because mutex_init itself calls malloc
mutex_t malloc_mutex = {.lock = 0};

/* safe versions of malloc functions */
void *malloc(size_t size)
{
    mutex_lock(&malloc_mutex);
    void *ret =  _malloc(size);
    mutex_unlock(&malloc_mutex);
    return ret;
}

void *memalign(size_t alignment, size_t size)
{
    mutex_lock(&malloc_mutex);
    void *ret =  _memalign(alignment, size);
    mutex_unlock(&malloc_mutex);
    return ret;
}

void *calloc(size_t nelt, size_t eltsize)
{
    mutex_lock(&malloc_mutex);
    void *ret =  _calloc(nelt, eltsize);
    mutex_unlock(&malloc_mutex);
    return ret;
}

void *realloc(void *buf, size_t new_size)
{
    mutex_lock(&malloc_mutex);
    void *ret =  _realloc(buf, new_size);
    mutex_unlock(&malloc_mutex);
    return ret;
}

void free(void *buf)
{
    mutex_lock(&malloc_mutex);
    _free(buf);
    mutex_unlock(&malloc_mutex);
}

void *smalloc(size_t size)
{
    mutex_lock(&malloc_mutex);
    void *ret =  _smalloc(size);
    mutex_unlock(&malloc_mutex);
    return ret;
}

void *smemalign(size_t alignment, size_t size)
{
    mutex_lock(&malloc_mutex);
    void *ret =  _smemalign(alignment, size);
    mutex_unlock(&malloc_mutex);
    return ret;
}

void sfree(void *buf, size_t size)
{
    mutex_lock(&malloc_mutex);
    _sfree(buf, size);
    mutex_unlock(&malloc_mutex);
}


