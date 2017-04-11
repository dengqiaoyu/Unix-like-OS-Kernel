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
#include <asm.h>    /* disable_interrupt(), enable... */

#include "vm.h"
#include "scheduler.h"
#include "asm_registers.h"
#include "asm_switch.h"
#include "allocator.h" /* allocator */
#include "asm_set_exec_context.h"
#include "utils.h"

extern id_counter_t id_counter;
extern allocator_t *sche_allocator;

extern uint32_t *kern_page_dir;
extern uint32_t zfod_frame;
extern int num_free_frames;

int kern_gettid(void) {
    thread_t *thread = get_cur_tcb();
    return thread->tid;
}

int kern_exec(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    char *execname = (char *)(*esi);
    char **argvec = (char **)(*(esi + 1));

    // TODO
    // need to check arg counts and lengths
    int argc = 0;
    char **temp = argvec;
    while (*temp) {
        argc += 1;
        temp += 1;
    }

    simple_elf_t elf_header;
    // TODO
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

    thread_t *thread = get_cur_tcb();
    /*
    // need to free old kernel stack?
    void *kern_stack = malloc(KERN_STACK_SIZE);
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    */
    thread->user_sp = USER_STACK_START;
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
    char **argv = (char **)(USER_STACK_START + 5 * sizeof(int));
    // 6 because argv is a null terminated char ptr array
    char *buf = (char *)(USER_STACK_START + 6 * sizeof(int) +
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

    uint32_t *ptr = (uint32_t *)USER_STACK_START;
    *(ptr + 1) = argc;
    *(ptr + 2) = (uint32_t)argv;
    *(ptr + 3) = USER_STACK_LOW + USER_STACK_SIZE;
    *(ptr + 4) = USER_STACK_LOW;

    // update fname for simics symbolic debugging
    sim_reg_process(task->page_dir, elf_header.e_fname);

    set_esp0(thread->kern_sp);
    kern_to_user(USER_STACK_START, elf_header.e_entry);

    return 0;
}

int kern_new_pages(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    uint32_t base = (uint32_t)(*esi);
    uint32_t len = (uint32_t)(*(esi + 1));

    if (base & (~PAGE_ALIGN_MASK)) {
        return -1;
    }

    if (len % PAGE_SIZE != 0 || len == 0) {
        return -1;
    }

    // this is an ugly check for "overflowing" len
    if (base + (len - 1) < base) {
        return -1;
    }

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    if (maps_find(task->maps, base, len)) {
        // already mapped or reserved
        return -1;
    }

    if (dec_num_free_frames(len / PAGE_SIZE) < 0) {
        // not enough memory
        return -1;
    }

    uint32_t page_addr;
    for (page_addr = base; page_addr - base > len; page_addr += PAGE_SIZE) {
        set_pte(page_addr, zfod_frame, PTE_USER | PTE_PRESENT);
    }

    maps_insert(task->maps, base, len, MAP_USER | MAP_WRITE);

    return 0;
}

int kern_deschedule(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    int *reject = (int *)esi;
    // if (check_ptr_valid((uint32_t)reject, (uint32_t *)get_cr3(), 0) == -1)
    //     return -1;

    disable_interrupts();
    if (*reject != 0) {
        enable_interrupts();
        return 0;
    }
    lprintf("%d\n", __LINE__);
    sche_yield(1);
    enable_interrupts();
    return 0;
}