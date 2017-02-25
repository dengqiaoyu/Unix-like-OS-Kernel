/**
 * @file allocator_internal.h
 * @brief This file contains the structure definition and some macros used
 *        internally.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bug
 */
#ifndef __ALLOCATOR_INTERNAL_H__
#define __ALLOCATOR_INTERNAL_H__

#include "allocator.h" /* allocator_t */
#include "mutex.h"     /* mutex_t */
#include "list.h"      /* node_t */

#define NUM_BITS_PER_BYTE 8

/* Ronud up x to the multiply of y */
#define ROUNDUP(x, y) ((unsigned int)((x) + (y) - 1) / (y) * (y))

/* see allocator.c for details */
typedef node_t allocator_node_t;
typedef struct allocator_block allocator_block_t;
typedef struct allocator_block {
    unsigned int chunk_num;
    unsigned int chunk_size;
    unsigned char bit_masks[MAX_CHUNK_NUM / 8];
    mutex_t allocator_block_mutex;
    char data[0];
};

/**
 * Get free chunk according to required_size.
 * @param  allocator_block the pointer to the block that has free chunk
 * @param  required_size   required size for free chunk
 * @return                 address of free chunk for success, NULL for fail
 */
void *get_free_chunk(allocator_block_t *allocator_block,
                     unsigned int required_size);

/**
 * Get the index of a block in the range of 8 chunk(8 bit to indicate).
 * @param  bit_mask              the bitmask that indicate free chunk
 * @param  bit_mask_ptr          the pointer to that bitmask
 * @param  allocator_block_mutex mutex for specific block
 * @return                       the index of a block in the range of 8
 *                               chunk(8 bit to indicate)
 */
unsigned int get_free_chunk_idx(unsigned char bit_mask,
                                unsigned char *bit_mask_ptr,
                                mutex_t *allocator_block_mutex);

/**
 * Malloc a new block and add it to the front of allocator list.
 * @param  allocator     the pointer to the allocator
 * @param  required_size the size of every chunk it will malloc
 * @param  new_chunk_num the number of chunks it will malloc
 * @return               SUCCESS(0) for success, negative number for fail
 */
int add_new_block_to_front(allocator_t *allocator,
                           unsigned int required_size,
                           unsigned int new_chunk_num);

/**
 * Get the pinter to the bitmask char which the chunk belongs to.
 * @param  chunk_ptr the pointer to the chunk
 * @return           the pinter to the bitmask char which the chunk belongs to
 */
unsigned char *get_bit_mask(void *chunk_ptr);

/**
 * Get the index of chunk inside a block
 * @param  chunk_ptr the pointer to the chunk
 * @return           the index of chunk inside a block
 */
unsigned int get_chunk_idx(void *chunk_ptr);

/**
 * Get the pointer to the block where the chunk belongs to.
 * @param  chunk_ptr the pointer to the chunk
 * @return           the pointer to the block where the chunk belongs to
 */
allocator_block_t *get_allocator_block(void *chunk_ptr);

#endif /* __ALLOCATOR_INTERNAL_H__ */