#include <malloc.h>
#include "error_type.h"
#include "list.h"
#include "allocator.h"
#include "allocator_pri.h"
#include "mutex.h"


int allocator_init(allocator_t **allocator,
                   unsigned int chunk_size,
                   unsigned int chunk_num) {
    int ret = SUCCESS;
    //BUG check return of malloc
    allocator_t *allocator_ptr = malloc(sizeof(allocator_t));
    allocator_ptr->list = init_list();
    ret = mutex_init(&(allocator_ptr->allocator_mutex));
    if (ret != SUCCESS) {
        return ERROR_ALLOCATOR_INIT_FAILED;
    }
    ret = add_new_block_to_front(allocator, chunk_size, chunk_num);
    if (ret != SUCCESS) {
        return ERROR_ALLOCATOR_INIT_FAILED;
    }

    return SUCCESS;
}

void *allocator_alloc(*allocator_t allocator,
                      unsigned int required_size,
                      unsigned int new_chunk_num) {
    allocator_node_t node_rover = get_first_node(allocator->list);
    while (node_rover != NULL) {
        allocator_block_t *allocator_block =
            (allocator_block_t *)node_rover->data;
        void *chunk_ptr = get_free_chunk(allocator_block, required_size);
        if (chunk_ptr != NULL) {
            return chunk_ptr;
        } else {
            node_rover = node_rover->next;
        }
    }

    /* Did not find free chunk */
    //TODO if add_new_block_to_front failed due to malloc
    add_new_block_to_front(allocator, required_size, new_chunk_num);
    void *chunk_ptr = get_free_chunk(get_first_node(allocator)->data,
                                     required_size);
    return chunk_ptr;
}

void allocator_free(void* chunk_ptr) {
    void *allocator_block = get_allocator_block(chunk_ptr);
    unsigned int idx = get_chunk_idx(chunk_ptr);
    unsigned int idx_mask = 1 << idx;
    unsigned int *bit_mask = get_bit_mask(chunk_ptr);
    *bit_masks = (*bit_mask) ^ idx_mask;
    mutex_unlock(allocator_block->allocator_block_mutex);
}

void *get_free_chunk(allocator_block_t *allocator_block,
                     unsigned int required_size) {
    int i;
    int chunk_size = allocator_block->chunk_size;
    if (chunk_size < required_size)
        return NULL;
    int chunk_num = allocator_block->chunk_num;
    int *bit_masks = allocator_block->bit_mask;

    int idx = -1;
    mutex_lock(allocator_block->mutex);
    for (i = 0; i < chunk_num; i += NUM_BITS_PER_BYTE) {
        unsigned int bit_mask = bit_masks[i];
        if (bit_mask == 0) {
            mutex_unlock(allocator_block->mutex);
            continue;
        }
        idx = get_free_chunk_idx(bit_mask, bit_masks + i);
        idx += i;
        break;
    }

    if (idx >= 0) {
        void *chunk_ptr = (allocator_block->data
                           + (chunk_size + sizeof(unsigned int)) * idx
                           + sizeof(allocator_block_t *));
        return chunk_ptr;
    } else {
        return NULL;
    }
}

unsigned int get_free_chunk_idx(unsigned int bit_mask,
                                unsigned int *bit_mask_ptr,
                                mutex_t *allocator_block_mutex) {
    unsigned int idx_mask = bit_mask ^ (bit_mask & (bit_mask - 1));
    *ptr_mask_ptr = bit_mask ^ idx_mask;
    mutex_unlock(allocator_block_mutex);
    unsigned int idx;
    while (bit_mask >>= 1)
        idx++;
    return idx;
}

int add_new_block_to_front(allocator_t *allocator,
                           unsigned int required_size,
                           unsigned int new_chunk_num) {
    unsigned int round_size =
        ROUNDUP((required_size + sizeof(allocator_block_t *)), ALIGNMENT);
    int block_node_size = (sizeof(allocator_node_t) - sizeof(char))
                          + (sizeof(allocator_block_t) - sizeof(char))
                          + round_size * new_chunk_num;
    int i, ret;
    node_t *block_node = malloc(node_size);
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
    unsigned int offset = (sizeof(allocator_block_t *) + required_size);
    for (i = 0; i < new_chunk_num; i++) {
        allocator_block_t *allocator_block_back_ptr = NULL;
        allocator_block_back_ptr = data + offset * i;
        *allocator_block_back_ptr = allocator_block;
    }
    mutex_lock(&(allocator->allocator_mutex));
    add_node_to_head(allocator, block_node);
    mutex_unlock(&(allocator->allocator_mutex));

    return SUCCESS;
}

inline unsigned int *get_bit_mask(void *chunk_ptr) {
    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    unsigned int idx = get_chunk_idx(chunk_ptr);
    mutex_lock(allocator_block->allocator_mutex);
    unsigned int *bit_mask = allocator_block->bit_masks[bit_mask_idx];
    return bit_mask;
}

inline unsigned int get_chunk_idx(void *chunk_ptr) {

    allocator_block_t *allocator_block = get_allocator_block(chunk_ptr);
    void *data = allocator_block->data;
    unsigned int idx =
        (chunk_ptr - data)
        / (allocator_block->chunk_size + sizeof(allocator_block_t *));
    return idx;
}

inline allocator_block_t *get_allocator_block(void *chunk_ptr) {
    return *(allocator_block_t *)(chunk_ptr - sizeof(allocator_block_t *))
}
