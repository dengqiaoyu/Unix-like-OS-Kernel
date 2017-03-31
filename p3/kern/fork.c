#include "syscalls.h"

static task_t  *_fork_task_init(task_t *old_task);
static int _fork_page_init(uint32_t **new_pgdir_ptr, uint32_t *old_pgdir);
static int _fork_thread_init(thread_t **main_thread_ptr, task_t *new_task);

int kern_fork(void) {
    int ret = SUCCESS;
    thread_t *old_thread = (GET_TCB(cur_sche_node));
    task_t *old_task = old_thread->task;
    if (old_task->thread_cnt != 1)
        return ERROR_FORK_TASK_MORE_THAN_ONE_THREAD;

    // init task
    task_t *new_task = _fork_task_init(old_task);
    if (new_task == NULL) return ERROR_FORK_MALLOC_TASK_FAILED;

    // init_paging
    ret = _fork_page_init(&(new_task->page_dir), old_task->page_dir);
    if (ret != SUCCESS) return ERROR_FORK_COPY_PAGE_FAILED;

    // init_thread
    ret = _fork_thread_init(&(new_task->main_thread), new_task);
    if (ret != SUCCESS) return ret;
    old_task->child_cnt++;

    thread_t *new_thread = new_task->main_thread;
    asm_set_exec_context(old_thread->kern_sp,
                         new_thread->kern_sp,
                         (uint32_t) & (new_thread->curr_esp),
                         (uint32_t) & (new_thread->ip));
    // Now, we will have two tasks running

    thread_t *curr_thr = GET_TCB(cur_sche_node);
    sche_node_t *sche_node = get_mainthr_sche_node(new_task);
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

int _fork_page_init(uint32_t **new_pgdir_ptr, uint32_t *old_pgdir) {
    *new_pgdir_ptr = (uint32_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
    uint32_t *new_pgdir = *new_pgdir_ptr;
    memset((void *)new_pgdir, 0, PAGE_SIZE);
    int new_pde_flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    uint32_t new_pde =
        (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | new_pde_flags;
    new_pgdir[RW_PHYS_PD_INDEX] = new_pde;
    int ret = copy_pgdir(new_pgdir, old_pgdir);
    if (ret != SUCCESS) {
        *new_pgdir_ptr = NULL;
        sfree((void *)new_pgdir, PAGE_SIZE);
        return ERROR_FORK_COPY_PAGE_FAILED;
    }

    return SUCCESS;
}

int _fork_thread_init(thread_t **main_thread_ptr, task_t *new_task) {
    sche_node_t *sche_node = allocator_alloc(sche_allocator);
    if (sche_node == NULL) {
        //free_page_dir(new_task->page_dir);
        sfree((void *)new_task->page_dir[1022], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        free(new_task);
        return ERROR_FORK_MALLOC_THREAD_FAILED;
    }
    thread_t *new_thread = GET_TCB(sche_node);
    mutex_lock(&id_counter.thread_id_counter_mutex);
    new_thread->tid = id_counter.thread_id_counter++;
    mutex_unlock(&id_counter.thread_id_counter_mutex);
    void *kern_stack = malloc(KERN_STACK_SIZE);
    if (kern_stack == NULL) {
        //free_page_dir(new_task->page_dir);
        sfree((void *)new_task->page_dir[1022], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        allocator_free(sche_node);
        free(new_task);
        return ERROR_FORK_MALLOC_KERNEL_STACK_FAILED;
    }
    new_thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    new_thread->user_sp = USER_STACK_LOW + USER_STACK_SIZE;
    new_thread->task = new_task;
    new_thread->status = FORKED;
    *main_thread_ptr = new_thread;
    return SUCCESS;
}

