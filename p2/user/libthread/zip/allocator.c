#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <simics.h>
#include "list.h"
#include "allocator.h"
#include "allocator_internal.h"
#include "mutex.h"

int allocator_init(allocator_t **allocator,
                   unsigned int chunk_size,
                   unsigned int chunk_num) {
    int ret;
    allocator_t *allocator_ptr = malloc(sizeof(allocator_t));
    if (allocator_ptr == NULL) return ERROR_ALLOCATOR_INIT_FAILED;

    allocator_ptr->list = init_list();
    ret = mutex_init(&(allocator_ptr->allocator_mutex));
    if (ret != SUCCESS) return ERROR_ALLOCATOR_INIT_FAILED;
    ret = add_new_block_to_front(allocator_ptr, chunk_size,
                                 ROUNDUP(chunk_num, NUM_BITS_PER_BYTE));
    if (ret != SUCCESS) return ERROR_ALLOCATOR_INIT_FAILED;

    *allocator = allocator_ptr;
    return SUCCESS;
}

void *allocator_alloc(allocator_t *allocator) {
    allocator_node_t *node_rover = get_first_node(allocator->list);
    allocator_block_t *allocator_block = (allocator_block_t *)(node_rover->data);
    unsigned int chunk_size = allocator_block->chunk_size;
    unsigned int chunk_num = allocator_block->chunk_num;
    while (node_rover != NULL) {
        allocator_block_t *allocator_block =
            (allocator_block_t *)node_rover->data;
        void *chunk_ptr = get_free_chunk(allocator_block, chunk_size);
        if (chunk_ptr != NULL) {
            return chunk_ptr;
        } else {
            node_rover = node_rover->next;
        }
    }

    /* Did not find free chunk */
    if (add_new_block_to_front(allocator, chunk_size, chunk_num) != SUCCESS) {
        return NULL;
    }
    void *chunk_ptr =
        get_free_chunk(
            (allocator_block_t *)get_first_node(allocator->list)->data,
            chunk_size);
    return chunk_ptr;
}

void allocator_free(void* chunk_ptr) {
    assert(chunk_ptr != NULL);
    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    unsigned int idx = get_chunk_idx(chunk_ptr);
    unsigned char idx_mask = 1 << (idx % NUM_BITS_PER_BYTE);
    unsigned char *bit_mask = get_bit_mask(chunk_ptr);
    *bit_mask = (*bit_mask) ^ idx_mask;
    mutex_unlock(&(allocator_block->allocator_block_mutex));
}

void *get_free_chunk(allocator_block_t *allocator_block,
                     unsigned int required_size) {
    int i;
    int chunk_size = allocator_block->chunk_size;
    if (chunk_size < required_size)
        return NULL;
    int chunk_num = allocator_block->chunk_num;
    unsigned char *bit_masks = allocator_block->bit_masks;

    int idx = -1;
    mutex_lock(&(allocator_block->allocator_block_mutex));
    for (i = 0; i < chunk_num / NUM_BITS_PER_BYTE; i += 1) {
        unsigned char bit_mask = bit_masks[i];
        if (bit_mask == 0) {
            mutex_unlock(&(allocator_block->allocator_block_mutex));
            continue;
        }
        idx = get_free_chunk_idx(bit_mask, bit_masks + i,
                                 &(allocator_block->allocator_block_mutex));
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
        return ERROR_ALLOCATOR_ADD_BLOCK_FAILED;
    }
    memset(block_node, 0, block_node_size);
    allocator_block_t *allocator_block = (allocator_block_t *)block_node->data;
    allocator_block->chunk_size = required_size;
    allocator_block->chunk_num = new_chunk_num;
    ret = mutex_init(&(allocator_block->allocator_block_mutex));
    if (ret != SUCCESS) {
        return ERROR_ALLOCATOR_ADD_BLOCK_FAILED;
    }
    memset(allocator_block->bit_masks, 0xff, MAX_CHUNK_NUM / NUM_BITS_PER_BYTE);
    void *data = allocator_block->data;
    unsigned int offset = (sizeof(allocator_block_t **) + required_size);
    for (i = 0; i < new_chunk_num; i++) {
        allocator_block_t **allocator_block_back_ptr = NULL;
        allocator_block_back_ptr = data + offset * i;
        *allocator_block_back_ptr = allocator_block;
    }
    mutex_lock(&(allocator->allocator_mutex));
    add_node_to_head(allocator->list, block_node);
    mutex_unlock(&(allocator->allocator_mutex));

    return SUCCESS;
}

unsigned char *get_bit_mask(void *chunk_ptr) {
    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    unsigned int idx = get_chunk_idx(chunk_ptr);
    unsigned int bit_mask_idx = idx / NUM_BITS_PER_BYTE;
    mutex_lock(&(allocator_block->allocator_block_mutex));
    unsigned char *bit_mask = &allocator_block->bit_masks[bit_mask_idx];
    return bit_mask;
}

unsigned int get_chunk_idx(void *chunk_ptr) {

    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    void *data = allocator_block->data;
    unsigned int idx =
        (chunk_ptr - data)
        / (allocator_block->chunk_size + sizeof(allocator_block_t **));
    return idx;
}

allocator_block_t *get_allocator_block(void *chunk_ptr) {
    return *(allocator_block_t **)(chunk_ptr - sizeof(allocator_block_t **));
}
