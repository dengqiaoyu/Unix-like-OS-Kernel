/**
 * @file allocator.h
 * @brief This file contains the structure definition and some macros used
 *        in public.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bug
 */

#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include "list.h"
#include <mutex.h>
#include "return_type.h"

/* max number of chunks inside a block */
#define MAX_CHUNK_NUM 1024

/* see allocator.c for details */
typedef struct allocator allocator_t;
struct allocator {
    list_t list;
    mutex_t allocator_mutex;
};

/**
 * Initialize the allocator and create the first block for future use.
 * @param  allocator  the address of allocator pointer, which we can change its
 *                    pointer value.
 * @param  chunk_size the chunk size for this allocator, once it is set, the
 *                    allocator will always return a chunk with such a size
 * @param  chunk_num  the number of chunk in a block, if allocator run out of
 *                    block it will malloc the same number of chunks that is set
 *                    when intialized.
 * @return            SUCCESS(0) for success, ERROR_ALLOCATOR_INIT_FAILED for
 *                    fail
 */
int allocator_init(allocator_t **allocator,
                   unsigned int chunk_size,
                   unsigned int chunk_num);
/**
 * Allocate a new chunk for future use.
 * @param  allocator the pointer to the allocator
 * @return           the address of memory for success, NULL for fail
 */
void *allocator_alloc(allocator_t *allocator);

/**
 * "Free" the chunk, here free is only giving back to the allocator by setting
 * bitmask to 1 which can be used for future allocate.
 * @param chunk_ptr the address of memory chunk
 */
void allocator_free(void *chunk_ptr);

/**
 * Clear the allocator, free all the blocks.
 * @param allocator the pointer to the allocator
 */
void destroy_allocator(allocator_t *allocator);

#endif /* __ALLOCATOR_H__ */
