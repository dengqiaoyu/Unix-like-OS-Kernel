#include "allocator.h"
#include "allocator_internal.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <simics.h>

void *stack_base[128] = {NULL};
allocator_t *allocator = NULL;
void print_bitmask(allocator_t *allocator);

int main() {
    allocator_init(&allocator, 4096, 512);
    int i = 0;
    // for (i = 0; ; i++) {
    //     void *stackbase = allocator_alloc(allocator);
    //     if (stackbase == NULL) {
    //         break;
    //     }
    //     if (i % 100 == 0)
    //         lprintf("%d", i);
    // }

    void *test = malloc(16783512);
    if (test != NULL) {
        lprintf("malloc works\n");
    }
    lprintf("%d stacks in total\n", i);
    // int i = 0;
    // for (i = 0; i < 128; i++) {
    //     stack_base[i] = allocator_alloc(allocator);
    //     if (stack_base[i] == NULL) {
    //         printf("fail\n");
    //     }
    // }
    print_bitmask(allocator);

    // for (i = 0; i < 64; i++) {
    //     allocator_free(stack_base[i]);
    // }

    // allocator_alloc(allocator);
    // print_bitmask(allocator);
}

void print_bitmask(allocator_t *allocator) {
    allocator_node_t *node_rover = get_first_node(&(allocator->list));
    int num_block = 0;
    while (node_rover->next != NULL) {
        num_block++;
        allocator_block_t *allocator_block =
            (allocator_block_t *)node_rover->data;
        int chunk_num = allocator_block->chunk_num;
        int i = 0;
        unsigned char *bit_masks = allocator_block->bit_masks;
        for (i = 0; i < chunk_num / NUM_BITS_PER_BYTE; i += 1) {
            unsigned char bit_mask = bit_masks[i];
            lprintf("%dth bit_mask: %x\n", i, bit_mask);
        }
        node_rover = node_rover->next;
    }

    lprintf("num_block: %d\n", num_block);
}