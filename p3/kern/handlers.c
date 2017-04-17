/** @file handlers.c
 *  @author Newton Xie (ncx) Qiaoyu Deng(qdeng)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <simics.h>
#include <x86/asm.h>
#include <x86/seg.h>
#include <x86/cr.h>
#include <x86/idt.h>
#include <syscall_int.h>
#include <x86/timer_defines.h>  /* TIMER_IDT_ENTRY */
#include <keyhelp.h>            /* KEY_IDT_ENTRY */
#include <interrupt_defines.h>  /* INT_ACK_CURRENT, INT_CTL_PORT */

#include "handlers.h"
#include "vm.h"
#include "task.h"
#include "syscalls/asm_syscalls.h"
#include "exceptions/asm_exceptions.h"
#include "drivers/asm_interrupts.h"
#include "drivers/timer_driver.h"
#include "drivers/keyboard_driver.h"
#include "asm_page_inval.h"
#include "scheduler.h"              /* sche_yield */

void timer_callback(unsigned int num_ticks);

// void pf_handler(int error_code) {
//     uint32_t pf_addr = get_cr2();
//     uint32_t pte = get_pte(pf_addr);

//     if ((pte & PAGE_ALIGN_MASK) == get_zfod_frame()) {
//         asm_page_inval((void *)pf_addr);
//         uint32_t frame_addr = get_frame();
//         set_pte(pf_addr, frame_addr, PTE_WRITE | PTE_USER | PTE_PRESENT);
//     } else {
//         lprintf("page fault: error_code: %d", error_code);
//         thread_t *tcb_ptr = get_cur_tcb();
//         task_t *cur_task_ptr = tcb_ptr->task;
//         kern_mutex_lock(&(cur_task_ptr->thread_list_mutex));
//         int live_threads = get_list_size(cur_task_ptr->live_thread_list);
//         kern_mutex_unlock(&(cur_task_ptr->thread_list_mutex));
//         if (live_threads == 1) cur_task_ptr->status = -2;
//         kern_vanish();
//     }

//     return;
// }
int handler_init() {
    exception_init();
    syscall_init();
    device_init();
    return 0;
}

int exception_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_KERNEL;
    idt_install(IDT_PF, asm_pagefault, kern_cs, flag);
    idt_install(IDT_DE, asm_divide,    kern_cs, flag);
    return 0;
}

int syscall_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_USER;
    idt_install(FORK_INT,           (void *)asm_fork,           kern_cs, flag);
    idt_install(EXEC_INT,           (void *)asm_exec,           kern_cs, flag);
    idt_install(WAIT_INT,           (void *)asm_wait,           kern_cs, flag);
    idt_install(YIELD_INT,          (void *)asm_yield,          kern_cs, flag);
    idt_install(DESCHEDULE_INT,     (void *)asm_deschedule,     kern_cs, flag);
    idt_install(MAKE_RUNNABLE_INT,  (void *)asm_make_runnable,  kern_cs, flag);
    idt_install(GETTID_INT,         (void *)asm_gettid,         kern_cs, flag);
    idt_install(NEW_PAGES_INT,      (void *)asm_new_pages,      kern_cs, flag);
    idt_install(REMOVE_PAGES_INT,   (void *)asm_remove_pages,   kern_cs, flag);
    idt_install(SLEEP_INT,          (void *)asm_sleep,          kern_cs, flag);
    idt_install(READLINE_INT,       (void *)asm_readline,       kern_cs, flag);
    idt_install(PRINT_INT,          (void *)asm_print,          kern_cs, flag);
    idt_install(SET_TERM_COLOR_INT, (void *)asm_set_term_color, kern_cs, flag);
    idt_install(SET_CURSOR_POS_INT, (void *)asm_set_cursor_pos, kern_cs, flag);
    idt_install(GET_CURSOR_POS_INT, (void *)asm_get_cursor_pos, kern_cs, flag);
    idt_install(THREAD_FORK_INT,    (void *)asm_thread_fork,    kern_cs, flag);
    idt_install(GET_TICKS_INT,      (void *)asm_get_ticks,      kern_cs, flag);
    idt_install(HALT_INT,           (void *)asm_halt,           kern_cs, flag);
    idt_install(SET_STATUS_INT,     (void *)asm_set_status,     kern_cs, flag);
    idt_install(VANISH_INT,         (void *)asm_vanish,         kern_cs, flag);
    idt_install(SWEXN_INT,          (void *)asm_swexn,          kern_cs, flag);
    return 0;
}

int device_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_KERNEL;
    timer_init(timer_callback);
    idt_install(TIMER_IDT_ENTRY, (void *)asm_timer_handler,    kern_cs, flag);
    idt_install(KEY_IDT_ENTRY,   (void *)asm_keyboard_handler, kern_cs, flag);
    return 0;
}

void idt_install(int idt_idx,
                 void (*entry)(),
                 int selector,
                 int flag) {
    uint32_t *idt_addr = (uint32_t *)idt_base() + 2 * idt_idx;
    *idt_addr = ((uint32_t)entry & 0x0000ffff) | (selector << 16);
    *(idt_addr + 1) =
        ((uint32_t)entry & 0xffff0000) | ((flag | FLAG_PRESENT) << 8);
}

/**
 * @brief Callback function that is called by the handler.
 * @param num_ticks the number of 10 ms that is triggered.
 */
void timer_callback(unsigned int num_ticks) {
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
    sche_yield(RUNNABLE);
}
