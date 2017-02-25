#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include "list.h"
#include <mutex.h>
#include "return_type.h"

#define MAX_CHUNK_NUM 1024

typedef struct allocator allocator_t;
struct allocator {
    list_t list;
    mutex_t allocator_mutex;
};

int allocator_init(allocator_t **allocator,
                   unsigned int chunk_size,
                   unsigned int chunk_num);

void *allocator_alloc(allocator_t *allocator);
void allocator_free(void *chunk_ptr);
void destroy_allocator(allocator_t *allocator);

#endif
