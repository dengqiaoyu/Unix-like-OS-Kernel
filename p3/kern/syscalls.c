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
