#include "task.h"
#include "allocator.h"


allocator_t *taskcb_allocator;
allocator_t *thrcb_allocator;

int task_init() {
    if (allocator_init(&taskcb_allocator) != SUCCESS)
        return ERROR_TASK_CONTROL_BLOCK_INIT_FAILED;
}

void