/** @file syscalls.c
 *
 */

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
// #include "asm_registers.h"
#include "asm_switch.h"
#include "allocator.h" /* allocator */
#include "asm_get_esp_eip.h"


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
    int ret = SUCCESS;
    set_cr3((uint32_t)kern_page_dir);
    task_t *old_task = (GET_TCB(cur_sche_node))->task;
    task_t *new_task = malloc(sizeof(task_t));
    if (new_task == NULL)
        return ERROR_FORK_MALLOC_TASK_FAILED;
    mutex_lock(&id_counter.task_id_counter_mutex);
    new_task->task_id = id_counter.task_id_counter++;
    mutex_unlock(&id_counter.task_id_counter_mutex);

    // BUG so this is not thread safe, right? or we can have a allocator making
    // it thread safe for us. And what if this one fails;
    new_task->page_dir = (uint32_t *)smemalign(PAGE_SIZE, PAGE_SIZE);

    memset(new_task->page_dir, 0, PAGE_SIZE);
    int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    new_task->page_dir[1022] =
        (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | flags;
    new_task->page_dir[1023] =
        (uint32_t)new_task->page_dir | flags;

    ret = copy_pgdir(new_task->page_dir, old_task->page_dir);
    if (ret != SUCCESS) {
        // BUG thread safe
        sfree(new_task->page_dir[1022], PAGE_SIZE);
        sfree(new_task->page_dir, PAGE_SIZE);
        return ERROR_FORK_COPY_FIR_FAILED;
    }

    sche_node_t *sche_node = allocator_alloc(sche_allocator);
    if (sche_node == NULL) {
        //free_page_dir(new_task->page_dir);
        sfree(new_task->page_dir[1022], PAGE_SIZE);
        sfree(new_task->page_dir, PAGE_SIZE);
        return ERROR_FORK_MALLOC_THREAD_FAILED;
    }
    thread_t *new_thread = GET_TCB(sche_node);
    mutex_lock(&id_counter.thread_id_counter_mutex);
    new_thread->tid = id_counter.thread_id_counter++;
    mutex_unlock(&id_counter.thread_id_counter_mutex);
    void *kern_stack = malloc(KERN_STACK_SIZE);
    if (kern_stack == NULL) {
        //free_page_dir(new_task->page_dir);
        sfree(new_task->page_dir[1022], PAGE_SIZE);
        sfree(new_task->page_dir, PAGE_SIZE);
        allocator_free(sche_node);
        return ERROR_FORK_MALLOC_KERNEL_STACK_FAILED;
    }
    new_thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    // need set user_sp
    new_thread->task = new_task;
    new_thread->status = FORKED;

    new_task->main_thread = new_thread;

    asm_get_esp_eip(&new_thread->user_sp, &new_thread->ip);
    // Now, we will have two task running

}

void kern_exec(void) {
    uint32_t *esi = 0;
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
    elf_load_helper(&elf_header, execname);

    thread_t *thread = GET_TCB(cur_sche_node);
    // need to free old kernel stack
    void *kern_stack = malloc(KERN_STACK_SIZE);
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    thread->user_sp = USER_STACK_LOW + USER_STACK_SIZE;
    thread->ip = elf_header.e_entry;

    task_t *task = thread->task;
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

    lprintf("p0\n");

    // need macros here badly
    char *buf = (char *)(USER_STACK_LOW + USER_STACK_SIZE + 6 * sizeof(int) +
                         argc * sizeof(int));
    char **argv = (char **)(USER_STACK_LOW + USER_STACK_SIZE + 5 * sizeof(int));

    char *arg;
    int len;
    int i;
    for (i = 0; i < argc; i++) {
        arg = *(argvec + i);
        len = strlen(arg);
        strncpy(buf, arg, len);
        buf[len] = '\0';
        argv[i] = buf;
        buf += len + 1;
    }
    argv[argc] = NULL;
    lprintf("p1\n");

    uint32_t *ptr = (uint32_t *)(USER_STACK_LOW + USER_STACK_SIZE);
    *(ptr + 1) = argc;
    *(ptr + 2) = (uint32_t)argv;
    *(ptr + 3) = USER_STACK_HIGH;
    *(ptr + 4) = USER_STACK_LOW;
    lprintf("p2\n");

    set_esp0(thread->kern_sp);
    lprintf("p3\n");
    load_program(&elf_header, task->page_dir);
    lprintf("p4\n");
    MAGIC_BREAK;
    kern_to_user(USER_STACK_LOW + USER_STACK_SIZE, elf_header.e_entry);
}
