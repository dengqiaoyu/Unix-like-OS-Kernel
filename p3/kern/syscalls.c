/** @file syscalls.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <page.h>
#include <simics.h>
#include <x86/cr.h>
#include <mutex.h>  /* mutex */
#include <string.h> /* memset */
#include <malloc.h> /* malloc, smemalign, sfree */

#include "vm.h"
#include "scheduler.h"
#include "asm_registers.h"
#include "asm_switch.h"
#include "allocator.h" /* allocator */
#include "asm_set_exec_context.h"

extern sche_node_t *cur_sche_node;
extern uint32_t *kern_page_dir;
extern id_counter_t id_counter;
extern allocator_t *sche_allocator;

int kern_gettid(void) {
    thread_t *thread = GET_TCB(cur_sche_node);
    return thread->tid;
}

// TODO maybe devide into sub functions
int kern_fork(void) {
    thread_t *thr_tmp_test = GET_TCB(cur_sche_node);
    lprintf("Hi, I am thread %d in kernel space\n", thr_tmp_test->tid);
    int ret = SUCCESS;
    // This should be done first.
    // set_cr3((uint32_t)kern_page_dir);
    thread_t *old_thread = (GET_TCB(cur_sche_node));
    task_t *old_task = old_thread->task;
    if (old_task->child_cnt != 0)
        return -1;
    lprintf("after checking child count\n");
    // malloc new task structure
    task_t *new_task = malloc(sizeof(task_t));
    if (new_task == NULL)
        return ERROR_FORK_MALLOC_TASK_FAILED;

    // get new task id
    mutex_lock(&id_counter.task_id_counter_mutex);
    new_task->task_id = id_counter.task_id_counter++;
    mutex_unlock(&id_counter.task_id_counter_mutex);

    lprintf("after get new task id, task id: %d", new_task->task_id);

    // create and copy page directory
    // BUG so this is not thread safe, right? or we can have a allocator making
    // it thread safe for us. And what if this one fails;
    new_task->page_dir = (uint32_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
    memset(new_task->page_dir, 0, PAGE_SIZE);
    int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    new_task->page_dir[1022] =
        (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | flags;
    new_task->page_dir[1023] =
        (uint32_t)new_task->page_dir | flags;
    lprintf("before copy_pgdir");
    ret = copy_pgdir(new_task->page_dir, old_task->page_dir);
    lprintf("after copy_pgdir");
    if (ret != SUCCESS) {
        // BUG thread safe
        // sfree((void *)new_task->page_dir[1022], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        return ERROR_FORK_COPY_FIR_FAILED;
    }

    sche_node_t *sche_node = allocator_alloc(sche_allocator);
    lprintf("after allocate sche_node");
    if (sche_node == NULL) {
        //free_page_dir(new_task->page_dir);
        sfree((void *)new_task->page_dir[1022], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        return ERROR_FORK_MALLOC_THREAD_FAILED;
    }
    thread_t *new_thread = GET_TCB(sche_node);
    mutex_lock(&id_counter.thread_id_counter_mutex);
    new_thread->tid = id_counter.thread_id_counter++;
    mutex_unlock(&id_counter.thread_id_counter_mutex);
    lprintf("after get tid, new tid: %d\n", new_thread->tid);
    void *kern_stack = malloc(KERN_STACK_SIZE);
    if (kern_stack == NULL) {
        //free_page_dir(new_task->page_dir);
        sfree((void *)new_task->page_dir[1022], PAGE_SIZE);
        sfree((void *)new_task->page_dir, PAGE_SIZE);
        allocator_free(sche_node);
        return ERROR_FORK_MALLOC_KERNEL_STACK_FAILED;
    }
    new_thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    // need set user_sp
    new_thread->task = new_task;
    new_thread->status = FORKED;

    new_task->main_thread = new_thread;
    new_task->parent_task = old_task;
    old_task->child_cnt = 1;

    lprintf("before asm_set_exec_context");
    lprintf("old_thread->kern_sp: %p", (void *)old_thread->kern_sp);
    lprintf("new_thread->kern_sp: %p", (void *)new_thread->kern_sp);
    lprintf("&(new_thread->curr_esp): %p", (void *)(&(new_thread->curr_esp)));
    lprintf("new_thread->curr_esp: %p", (void *)new_thread->curr_esp);
    lprintf("&(new_thread->ip): %p", (void *)(&(new_thread->ip)));
    lprintf("new_thread->ip: %p", (void *)new_thread->ip);
    // MAGIC_BREAK;
    asm_set_exec_context(old_thread->kern_sp,
                         new_thread->kern_sp,
                         (uint32_t) & (new_thread->curr_esp),
                         (uint32_t) & (new_thread->ip));
    // lprintf("&(new_thread->curr_esp): %p", (void *)(&(new_thread->curr_esp)));
    // lprintf("new_thread->curr_esp: %p", (void *)new_thread->curr_esp);
    // lprintf("&(new_thread->ip): %p", (void *)(&(new_thread->ip)));
    // lprintf("new_thread->ip: %p", (void *)new_thread->ip);
    // MAGIC_BREAK;
    // asm_set_exec_context(0,
    //                      1,
    //                      2,
    //                      3);
    lprintf("after asm_set_exec_context");
    // Now, we will have two tasks running

    lprintf("begin return\n");
    thread_t *curr_thr = GET_TCB(cur_sche_node);
    if (curr_thr->task->child_cnt == 0) {
        lprintf("I am new task!");
        MAGIC_BREAK;
        return 0;
    } else {
        append_to_scheduler(sche_node);
        lprintf("I am your father!");
        return new_thread->tid;
    }
}

void kern_exec(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    char *execname = (char *)(*esi);
    char **argvec = (char **)(*(esi + 1));

    // need to check arg counts and lengths
    int argc = 0;
    char **temp = argvec;
    while (*temp) {
        argc += 1;
        temp += 1;
    }

    simple_elf_t elf_header;
    // really bad code, just for testing
    char namebuf[32];
    sprintf(namebuf, "%s", execname);
    elf_load_helper(&elf_header, namebuf);

    thread_t *thread = GET_TCB(cur_sche_node);
    // need to free old kernel stack
    void *kern_stack = malloc(KERN_STACK_SIZE);
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    thread->user_sp = USER_STACK_LOW + USER_STACK_SIZE;
    thread->ip = elf_header.e_entry;

    task_t *task = thread->task;
    int i;
    /*
    // memory is leaking
    // need to dispose of old mapped frames
    task->page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    // lprintf("task page dir is %p\n", task->page_dir);
    int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    task->page_dir[1022] = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | flags;
    task->page_dir[1023] = (uint32_t)task->page_dir | flags;

    for (i = 0; i < NUM_KERN_TABLES; i++) {
        task->page_dir[i] = kern_page_dir[i];
    }
    */

    // need macros here badly
    char *buf = (char *)(USER_STACK_LOW + USER_STACK_SIZE + 6 * sizeof(int) +
                         argc * sizeof(int));
    char **argv = (char **)(USER_STACK_LOW + USER_STACK_SIZE + 5 * sizeof(int));

    char *arg;
    int len;
    for (i = 0; i < argc; i++) {
        arg = *(argvec + i);
        len = strlen(arg);
        strncpy(buf, arg, len);
        buf[len] = '\0';
        argv[i] = buf;
        buf += len + 1;
    }
    argv[argc] = NULL;

    uint32_t *ptr = (uint32_t *)(USER_STACK_LOW + USER_STACK_SIZE);
    *(ptr + 1) = argc;
    *(ptr + 2) = (uint32_t)argv;
    *(ptr + 3) = USER_STACK_HIGH;
    *(ptr + 4) = USER_STACK_LOW;

    set_esp0(thread->kern_sp);
    load_program(&elf_header, task->page_dir);
    lprintf("cp");
    kern_to_user(USER_STACK_LOW + USER_STACK_SIZE, elf_header.e_entry);
}
