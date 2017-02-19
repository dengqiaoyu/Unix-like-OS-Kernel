#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include "list.h"
#include "mutex.h"

#define MAX_CHUNK_NUM 1024

struct allocator_t {
    list_t *list;
    mutex_t allocator_mutex;
} allocator_t;

int allocator_init(allocator_t **allocator,
                   unsigned int chunk_size,
                   unsigned int chunk_num);
void *allocator_alloc(*allocator_t allocator,
                      unsigned int required_size,
                      unsigned int new_chunk_num);
void allocator_free(*allocator_t allocator, void *chunk_ptr);

#endif