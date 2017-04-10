#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <page.h>
#include <simics.h>
#include <malloc.h> /* malloc, smemalign, sfree */

#include "vm.h"
#include "scheduler.h"
#include "mutex.h"
#include "asm_registers.h"
#include "asm_switch.h"
#include "allocator.h" /* allocator */
#include "task.h"
#include "asm_set_exec_context.h"

extern sche_node_t *cur_sche_node;
extern allocator_t *sche_allocator;

int kern_fork(void) {
    int ret = SUCCESS;
    // Do we need a mutex to protect this one?
    thread_t *old_thread = (GET_TCB(cur_sche_node));
    int old_tid = old_thread->tid;
    task_t *old_task = old_thread->task;
    thread_t *cur_thr = NULL;
    sche_node_t *sche_node = NULL;
    if (get_list_size(old_task->thread_list) != 1)
        return ERROR_FORK_TASK_MORE_THAN_ONE_THREAD;

    task_t *new_task = task_init();
    if (new_task == NULL) return ERROR_FORK_MALLOC_TASK_FAILED;
    new_task->parent_task = old_task;

    ret = copy_pgdir(new_task->page_dir, old_task->page_dir);
    if (ret != SUCCESS) {
        // TODO destroy task
        return ERROR_FORK_COPY_PAGE_FAILED;
    }

    // TODO copy maps

    // init_thread
    thread_t *new_thread = thread_init();
    if (new_thread == NULL) {
        // TODO destroy task
        // TODO unmap new page directory
        return -1;
    }
    new_task->task_id = new_thread->tid;
    new_thread->task = new_task;
    new_thread->status = FORKED;
    add_node_to_head(new_task->thread_list, TCB_TO_NODE(new_thread));

    add_node_to_head(old_task->child_task_list, PCB_TO_NODE(new_task));
    asm_set_exec_context(old_thread->kern_sp,
                         new_thread->kern_sp,
                         &(new_thread->cur_sp),
                         &(new_thread->ip));

    // Now, we will have two tasks running
    // BUG that has been found!!! cannot declare var here, because we will break
    // stack
    // Do we need a mutex to protect this one?
    cur_thr = GET_TCB(cur_sche_node);
    sche_node = GET_SCHE_NODE(new_thread);
    // if (get_list_size(cur_thr->task->child_task_list) == 0) {
    if (cur_thr->tid != old_tid) {
        return 0;
    } else {
        append_to_scheduler(sche_node);
        return new_thread->tid;
    }
}
