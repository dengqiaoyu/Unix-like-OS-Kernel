#include "syscalls.h"

task_t  *_fork_task_init(task_t *old_task);
int _fork_page_init(task_t *new_task, task_t *old_task);
// static int _fork_thread_init(thread_t **main_thread_ptr, task_t *new_task);
int temp_func(task_t *new_task, task_t *old_task);
sche_node_t *temp_func1(task_t *new_task);
thread_t *temp_func2(sche_node_t *sche_node,
                     task_t *new_task,
                     thread_t *new_thread);
int set_kern_stack(task_t *new_task, thread_t *new_thread);

int kern_fork(void) {
    // int ret = SUCCESS;
    thread_t *old_thread = (GET_TCB(cur_sche_node));
    task_t *old_task = old_thread->task;
    if (old_task->thread_cnt != 1)
        return ERROR_FORK_TASK_MORE_THAN_ONE_THREAD;

    // init task
    task_t *new_task = _fork_task_init(old_task);
    if (new_task == NULL) return ERROR_FORK_MALLOC_TASK_FAILED;

    // init_paging
    _fork_page_init(new_task, old_task);
    // new_task->page_dir = (uint32_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
    int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    uint32_t entry = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | flags;
    new_task->page_dir[RW_PHYS_PD_INDEX] = entry;
    temp_func(new_task, old_task);

    // init thread
    sche_node_t *sche_node = temp_func1(new_task);
    thread_t *new_thread = temp_func2(sche_node, new_task, NULL);



    new_thread->user_sp = USER_STACK_LOW + USER_STACK_SIZE;
    new_thread->task = new_task;
    new_thread->status = FORKED;
    new_task->main_thread = new_thread;
    old_task->child_cnt = 1;
    asm_set_exec_context(old_thread->kern_sp,
                         new_thread->kern_sp,
                         (uint32_t) & (new_thread->curr_esp),
                         (uint32_t) & (new_thread->ip));
    // Now, we will have two tasks running

    thread_t *curr_thr = GET_TCB(cur_sche_node);
    if (curr_thr->task->child_cnt == 0) {
        return 0;
    } else {
        append_to_scheduler(sche_node);
        return new_thread->tid;
    }
}



task_t *_fork_task_init(task_t *old_task) {
    task_t *new_task = malloc(sizeof(task_t));
    if (new_task == NULL) return NULL;
    mutex_lock(&id_counter.task_id_counter_mutex);
    new_task->task_id = id_counter.task_id_counter++;
    mutex_unlock(&id_counter.task_id_counter_mutex);
    new_task->parent_task = old_task;
    new_task->child_cnt = 0;
    new_task->thread_cnt = 1;
    return new_task;
}

int _fork_page_init(task_t *new_task, task_t *old_task) {
    new_task->page_dir = (uint32_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
    memset(new_task->page_dir, 0, PAGE_SIZE);
    int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    uint32_t entry = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | flags;
    new_task->page_dir[RW_PHYS_PD_INDEX] = entry;
    return SUCCESS;
}

int temp_func(task_t *new_task, task_t *old_task) {
    lprintf("before copy_pgdir");
    int ret = copy_pgdir(new_task->page_dir, old_task->page_dir);
    lprintf("after copy_pgdir");
    if (ret != SUCCESS) {
        sfree((void *)new_task->page_dir[RW_PHYS_PD_INDEX], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        return ERROR_FORK_COPY_PAGE_FAILED;
    }
    return SUCCESS;
}

sche_node_t *temp_func1(task_t *new_task) {
    sche_node_t *sche_node = allocator_alloc(sche_allocator);
    if (sche_node == NULL) {
        //free_page_dir(new_task->page_dir);
        lprintf("enter in");
        sfree((void *)new_task->page_dir[RW_PHYS_PD_INDEX], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        return NULL;
    }

    return sche_node;
}

thread_t *temp_func2(sche_node_t *sche_node,
                     task_t *new_task,
                     thread_t *new_thread) {
    new_thread = GET_TCB(sche_node);
    mutex_lock(&id_counter.thread_id_counter_mutex);
    new_thread->tid = id_counter.thread_id_counter++;
    mutex_unlock(&id_counter.thread_id_counter_mutex);
    return new_thread;
}

int set_kern_stack(task_t *new_task, thread_t *new_thread) {
    void *kern_stack = malloc(KERN_STACK_SIZE);
    if (kern_stack == NULL) {
        //free_page_dir(new_task->page_dir);
        sfree((void *)new_task->page_dir[1023], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        return -1;
    }
    new_thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    return SUCCESS;
}

// int _fork_thread_init(thread_t **main_thread_ptr, task_t *new_task) {
//     sche_node_t *sche_node = allocator_alloc(sche_allocator);
//     if (sche_node == NULL) {
//         //free_page_dir(new_task->page_dir);
//         sfree((void *)new_task->page_dir[1022], PAGE_SIZE);
//         sfree((void *)new_task->page_dir, PAGE_SIZE);
//         free(new_task);
//         return ERROR_FORK_MALLOC_THREAD_FAILED;
//     }
//     thread_t *new_thread = GET_TCB(sche_node);
//     mutex_lock(&id_counter.thread_id_counter_mutex);
//     new_thread->tid = id_counter.thread_id_counter++;
//     mutex_unlock(&id_counter.thread_id_counter_mutex);
//     void *kern_stack = malloc(KERN_STACK_SIZE);
//     if (kern_stack == NULL) {
//         //free_page_dir(new_task->page_dir);
//         sfree((void *)new_task->page_dir[1022], PAGE_SIZE);
//         sfree((void *)new_task->page_dir, PAGE_SIZE);
//         allocator_free(sche_node);
//         free(new_task);
//         return ERROR_FORK_MALLOC_KERNEL_STACK_FAILED;
//     }
//     new_thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
//     new_thread->user_sp = USER_STACK_LOW + USER_STACK_SIZE;
//     new_thread->task = new_task;
//     new_thread->status = FORKED;
//     *main_thread_ptr = new_thread;
//     return SUCCESS;
// }
//
