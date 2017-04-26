/**
 * @file allocator.c
 * @brief This file contains the definition of allocator function for allocating
 * fixed size of free memory chunk to the caller, which is used by thread lib.
 * The purpose of the allocator is that we want pre-malloc a relatively big
 * chunk of memory for future use, without calling malloc every time when we
 * need a new chunk, since malloc is protected by the single mutex, which is not
 * efficient when every thread is mallocing at the same time. And this allocator
 * can autoincrease the memory we can have by creating a block list, inside the
 * block there is multiple chunk memory that can be assigned to thread.
 * the allocator itself is thread-safe by intrducing mutex in the allocator and
 * the specific block.
 * An allocator is orgnized like this:
 * |alloctor|
 *     |-->list|-->node_cnt
 *             |-->head_node->|block|
 *     |->mutex|                 V
 *             |              |block|
 *             |                 V
 *             |              |block|
 *             |                 V
 *             |              |block|
 *             |                 V
 *             |-->tail_node->|block|
 * A block is like this:
 * --------------------------------------------------------------------------
 * | prev | next | chunk_num | chunk_size | bitmask | chunk1 | chunk2 | ... |
 * --------------------------------------------------------------------------
 * Inside a chunk is like this:
 * -------------------------------------------
 * |pointer to the head of block| data field |
 * -------------------------------------------
 * To clarify, bitmask is used to mask which is free, we use 1 bit in bitmask
 * to indicate the utilization of each chunk.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bug
 */

#include <malloc.h>             /* malloc() */
#include <string.h>             /* memset() */
#include <assert.h>             /* assert() */
#include "list.h"               /* list_t and its operation function*/
#include "allocator.h"          /* the public header file for this code */
#include "allocator_internal.h" /* the internal header file for this code */
#include "mutex.h"              /* mutex_t and its function */

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
                   unsigned int chunk_num) {
    int ret;
    allocator_t *allocator_ptr = malloc(sizeof(allocator_t));
    if (allocator_ptr == NULL) return ERROR_ALLOCATOR_INIT_FAILED;

    ret = init_list(&(allocator_ptr->list));
    if (ret != SUCCESS) {
        free(allocator_ptr);
        return ERROR_ALLOCATOR_INIT_FAILED;
    }
    ret = mutex_init(&(allocator_ptr->allocator_mutex));
    if (ret != SUCCESS) {
        free(allocator_ptr);
        return ERROR_ALLOCATOR_INIT_FAILED;
    }
    /*
     * malloc new block to the very front of list, chunk_num need to be multiply
     * of NUM_BITS_PER_BYTE(8), since we use 8 bits to represents the 8 chunks'
     * utilization status.
     */
    ret = add_new_block_to_front(allocator_ptr, chunk_size,
                                 ROUNDUP(chunk_num, NUM_BITS_PER_BYTE));
    if (ret != SUCCESS) {
        free(allocator_ptr);
        return ERROR_ALLOCATOR_INIT_FAILED;
    }

    *allocator = allocator_ptr;
    return SUCCESS;
}

/**
 * Allocate a new chunk for future use.
 * @param  allocator the pointer to the allocator
 * @return           the address of memory for success, NULL for fail
 */
void *allocator_alloc(allocator_t *allocator) {
    allocator_node_t *node_rover = get_first_node(&(allocator->list));
    /* get the block info */
    allocator_block_t *allocator_block =
        (allocator_block_t *)(node_rover->data);
    unsigned int chunk_size = allocator_block->chunk_size;
    unsigned int chunk_num = allocator_block->chunk_num;
    /*
     * iterate the whole list, the head_node and tail_node is set to zero as
     * dummy nodes.
     */
    while (node_rover->next != NULL) {
        allocator_block = (allocator_block_t *)node_rover->data;
        void *chunk_ptr = get_free_chunk(allocator_block, chunk_size);
        if (chunk_ptr != NULL) {
            return chunk_ptr;
        } else {
            node_rover = node_rover->next;
        }
    }

    /* did not find free chunk via malloc */
    if (add_new_block_to_front(allocator, chunk_size, chunk_num) != SUCCESS) {
        return NULL;
    }
    void *chunk_ptr =
        get_free_chunk(
            (allocator_block_t *)get_first_node(&(allocator->list))->data,
            chunk_size);
    return chunk_ptr;
}

/**
 * "Free" the chunk, here free is only giving back to the allocator by setting
 * bitmask to 1 which can be used for future allocate.
 * @param chunk_ptr the address of memory chunk
 */
void allocator_free(void* chunk_ptr) {
    assert(chunk_ptr != NULL);
    /* get the pointer to the block which this chunk belongs to */
    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    /* get the index of that chunk inside the block */
    unsigned int idx = get_chunk_idx(chunk_ptr);
    /* get the bitmask of maskbit field inside a single maskbit char(8 bit)*/
    unsigned char idx_mask = 1 << (idx % NUM_BITS_PER_BYTE);
    /* get the pointer to a single maskbit char(8 bit) */
    unsigned char *bit_mask = get_bit_mask(chunk_ptr);
    /* set mask from 0(used) to 1(free) */
    *bit_mask = (*bit_mask) ^ idx_mask;
    mutex_unlock(&(allocator_block->allocator_block_mutex));
}

/**
 * Clear the allocator, free all the blocks.
 * @param allocator the pointer to the allocator
 */
void destroy_allocator(allocator_t *allocator) {
    clear_list(&(allocator->list));
    mutex_destroy(&(allocator->allocator_mutex));
}

/**
 * Get free chunk according to required_size.
 * @param  allocator_block the pointer to the block that has free chunk
 * @param  required_size   required size for free chunk
 * @return                 address of free chunk for success, NULL for fail
 */
void *get_free_chunk(allocator_block_t *allocator_block,
                     unsigned int required_size) {
    int i;
    int chunk_size = allocator_block->chunk_size;
    /*
     * this will not happen for current implementationm, for further use, we
     * need a better way allocate variable chunk size
     */
    if (chunk_size < required_size)
        return NULL;
    int chunk_num = allocator_block->chunk_num;
    unsigned char *bit_masks = allocator_block->bit_masks;

    int idx = -1;
    mutex_lock(&(allocator_block->allocator_block_mutex));
    /* Iterate the bit_masks to see if there is one bit that is 1(free) */
    for (i = 0; i < chunk_num / NUM_BITS_PER_BYTE; i += 1) {
        unsigned char bit_mask = bit_masks[i];
        /* all of chunk is in use */
        if (bit_mask == 0) {
            mutex_unlock(&(allocator_block->allocator_block_mutex));
            continue;
        }
        /* get the index of chunk in the range of 8 chunk(8 bit to indicate) */
        idx = get_free_chunk_idx(bit_mask, bit_masks + i,
                                 &(allocator_block->allocator_block_mutex));
        /* the total index in a block */
        idx += i * NUM_BITS_PER_BYTE;
        break;
    }

    if (idx >= 0) {
        void *chunk_ptr = (allocator_block->data
                           + (chunk_size + sizeof(allocator_block_t **)) * idx
                           + sizeof(allocator_block_t **));
        return chunk_ptr;
    } else {
        return NULL;
    }
}

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
                                mutex_t *allocator_block_mutex) {
    unsigned char idx_mask = bit_mask ^ (bit_mask & (bit_mask - 1));
    *bit_mask_ptr = bit_mask ^ idx_mask;
    mutex_unlock(allocator_block_mutex);
    unsigned int idx = 0;
    while (idx_mask >>= 1)
        idx++;
    return idx;
}

/**
 * Malloc a new block and add it to the front of allocator list.
 * @param  allocator     the pointer to the allocator
 * @param  required_size the size of every chunk it will malloc
 * @param  new_chunk_num the number of chunks it will malloc
 * @return               SUCCESS(0) for success, negative number for fail
 */
int add_new_block_to_front(allocator_t *allocator,
                           unsigned int required_size,
                           unsigned int new_chunk_num) {
    int block_node_size = (sizeof(allocator_node_t))
                          + (sizeof(allocator_block_t))
                          + (sizeof(allocator_block_t **) + required_size)
                          * new_chunk_num;
    int i, ret;
    allocator_node_t *block_node = malloc(block_node_size);
    if (block_node == NULL) {
        return ERROR_ALLOCATOR_MALLOC_NEW_BLOCK_FAILED;
    }
    memset(block_node, 0, block_node_size);
    allocator_block_t *allocator_block = (allocator_block_t *)block_node->data;
    allocator_block->chunk_size = required_size;
    allocator_block->chunk_num = new_chunk_num;
    ret = mutex_init(&(allocator_block->allocator_block_mutex));
    if (ret != SUCCESS) {
        free(block_node);
        return ERROR_ALLOCATOR_ADD_BLOCK_FAILED;
    }
    /* set all the bitmasks to 1 */
    memset(allocator_block->bit_masks, 0xff, MAX_CHUNK_NUM / NUM_BITS_PER_BYTE);
    void *data = allocator_block->data;
    unsigned int offset = (sizeof(allocator_block_t **) + required_size);
    /* add the poniter to the block's begining for every chunk */
    for (i = 0; i < new_chunk_num; i++) {
        allocator_block_t **allocator_block_back_ptr = NULL;
        allocator_block_back_ptr = data + offset * i;
        *allocator_block_back_ptr = allocator_block;
    }
    mutex_lock(&(allocator->allocator_mutex));
    add_node_to_head(&(allocator->list), block_node);
    mutex_unlock(&(allocator->allocator_mutex));

    return SUCCESS;
}

/**
 * Get the pinter to the bitmask char which the chunk belongs to.
 * @param  chunk_ptr the pointer to the chunk
 * @return           the pinter to the bitmask char which the chunk belongs to
 */
unsigned char *get_bit_mask(void *chunk_ptr) {
    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    unsigned int idx = get_chunk_idx(chunk_ptr);
    unsigned int bit_mask_idx = idx / NUM_BITS_PER_BYTE;
    mutex_lock(&(allocator_block->allocator_block_mutex));
    unsigned char *bit_mask = &allocator_block->bit_masks[bit_mask_idx];
    return bit_mask;
}

/**
 * Get the index of chunk inside a block
 * @param  chunk_ptr the pointer to the chunk
 * @return           the index of chunk inside a block
 */
unsigned int get_chunk_idx(void *chunk_ptr) {

    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    void *data = allocator_block->data;
    unsigned int idx =
        (chunk_ptr - data)
        / (allocator_block->chunk_size + sizeof(allocator_block_t **));
    return idx;
}

/**
 * Get the pointer to the block where the chunk belongs to.
 * @param  chunk_ptr the pointer to the chunk
 * @return           the pointer to the block where the chunk belongs to
 */
allocator_block_t *get_allocator_block(void *chunk_ptr) {
    return *(allocator_block_t **)(chunk_ptr - sizeof(allocator_block_t **));
}
