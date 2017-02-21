#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include "list.h"
#include "mutex.h"

#define MAX_CHUNK_NUM 1024

#define ERROR_ALLOCATOR_INIT_FAILED -1
#define ERROR_ALLOCATOR_ADD_BLOCK_FAILED -2

typedef struct allocator allocator_t;
struct allocator {
    list_t *list;
    mutex_t allocator_mutex;
};

int allocator_init(allocator_t **allocator,
                   unsigned int chunk_size,
                   unsigned int chunk_num);

void *allocator_alloc(allocator_t *allocator);

void allocator_free(void *chunk_ptr);

#endif
