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
    // macro for namebuf length
    // need to check execname length fits
    char namebuf[32];
    sprintf(namebuf, "%s", execname);
    elf_load_helper(&elf_header, namebuf);

    char argbuf[256];
    char *ptrbuf[16];
    char *arg;
    char *marker = argbuf;
    int len;
    int i;
    for (i = 0; i < argc; i++) {
        arg = *(argvec + i);
        len = strlen(arg);
        strncpy(marker, arg, len);
        marker[len] = '\0';
        ptrbuf[i] = marker;
        marker += len + 1;
    }
    ptrbuf[argc] = NULL;

    thread_t *thread = GET_TCB(cur_sche_node);
    /*
    // need to free old kernel stack
    void *kern_stack = malloc(KERN_STACK_SIZE);
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    */
    thread->user_sp = USER_STACK_LOW + USER_STACK_SIZE;
    thread->ip = elf_header.e_entry;

    task_t *task = thread->task;
    maps_destroy(task->maps);
    task->maps = maps_init();
    maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES, 0);
    maps_insert(task->maps, RW_PHYS_VA, PAGE_SIZE, 0);

    clear_pgdir(task->page_dir);
    set_cr3((uint32_t)task->page_dir);

    load_program(&elf_header, task->maps);

    // need macros here badly
    // 5 because ret addr and 4 args
    char **argv = (char **)(USER_STACK_LOW + USER_STACK_SIZE + 5 * sizeof(int));
    // 6 because argv is a null terminated char ptr array
    char *buf = (char *)(USER_STACK_LOW + USER_STACK_SIZE + 6 * sizeof(int) +
                         argc * sizeof(int));

    for (i = 0; i < argc; i++) {
        arg = *(ptrbuf + i);
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

    // update fname for simics symbolic debugging
    sim_reg_process(task->page_dir, elf_header.e_fname);

    set_esp0(thread->kern_sp);
    kern_to_user(USER_STACK_LOW + USER_STACK_SIZE, elf_header.e_entry);
}
