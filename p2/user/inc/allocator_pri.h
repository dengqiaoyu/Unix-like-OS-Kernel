#ifndef __ALLOCATOR_PRIVATE_H__
#define __ALLOCATOR_PRIVATE_H__

#include "allocator.h"
#include "mutex.h"

#define ALIGNMENT 16
#define CHUNK_PADDING 3
#define NUM_BITS_PER_BYTE 8

#define ROUNDUP(x, y) ((unsigned int)((x) + (y) - 1) / (y) * (y))

typedef allocator_node_t node_t;
struct allocator_block_t {
    unsigned int chunk_num;
    unsigned int chunk_size;
    unsigned int bit_masks[MAX_CHUNK_NUM / 8];
    mutex_t allocator_block_mutex;
    unsigned int padding;
    char data[0];
} allocator_block_t;

void *get_free_chunk(allocator_block_t *allocator_block,
                     unsigned int required_size);
unsigned int get_free_chunk_idx(unsigned int bit_mask,
                                unsigned int *bit_mask_ptr);
//TODO if malloc fails.
int add_new_block_to_front(allocator_t allocator,
                           unsigned int required_size,
                           unsigned int new_chunk_num);
inline unsigned int *get_bit_mask(void *chunk_ptr);
inline unsigned int get_chunk_idx(void *chunk_ptr);
inline allocator_block_t *get_allocator_block(void *chunk_ptr);

#endif