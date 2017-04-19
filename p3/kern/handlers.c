/**
 *  @file   handlers.c
 *  @brief  This file contains the excepton handler, syscalls installer and
 *          keyboard and timer driver.
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng(qdeng)
 *  @bug    No known bugs.
 */
/* libc includes */
#include <stdlib.h>
#include <syscall_int.h>
#include <keyhelp.h>            /* KEY_IDT_ENTRY */
#include <interrupt_defines.h>  /* INT_ACK_CURRENT, INT_CTL_PORT */

/* x86 specific includes */
#include <x86/asm.h>
#include <x86/seg.h>
#include <x86/cr.h>
#include <x86/idt.h>
#include <x86/timer_defines.h>  /* TIMER_IDT_ENTRY */

#include "handlers.h"
#include "vm.h"
#include "task.h"
#include "asm_page_inval.h"
#include "scheduler.h"              /* sche_yield */
#include "syscalls/asm_syscalls.h"
#include "exceptions/asm_exceptions.h"
#include "drivers/asm_interrupts.h"
#include "drivers/timer_driver.h"
#include "drivers/keyboard_driver.h"

/* DEBUG */
#include <simics.h>

/**
 * @brief Callback function that is called by the handler.
 * @param num_ticks the number of 10 ms that is triggered.
 */
void timer_callback(unsigned int num_ticks);

/**
 * Initialize all handlers and system calls.
 * @return 0 as success
 */
int handler_init() {
    exception_init();
    syscall_init();
    device_init();
    return 0;
}

/**
 * Install all exception handlers.
 * @return 0 as success
 */
int exception_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_KERNEL;

    idt_install(IDT_DE,  asm_divide,        kern_cs, flag);
    idt_install(IDT_DB,  asm_debug,         kern_cs, flag);
    idt_install(IDT_NMI, asm_nonmaskable,   kern_cs, flag);
    idt_install(IDT_BP,  asm_break_point,   kern_cs, flag);
    idt_install(IDT_OF,  asm_overflow,      kern_cs, flag);
    idt_install(IDT_BR,  asm_boundcheck,    kern_cs, flag);
    idt_install(IDT_UD,  asm_opcode,        kern_cs, flag);
    idt_install(IDT_NM,  asm_nofpu,         kern_cs, flag);
    idt_install(IDT_CSO, asm_coproc,        kern_cs, flag);
    idt_install(IDT_TS,  asm_task_segf,     kern_cs, flag);
    idt_install(IDT_NP,  asm_segfault,      kern_cs, flag);
    idt_install(IDT_SS,  asm_stackfault,    kern_cs, flag);
    idt_install(IDT_GP,  asm_protfault,     kern_cs, flag);
    idt_install(IDT_PF,  asm_pagefault,     kern_cs, flag);
    idt_install(IDT_MF,  asm_fpufault,      kern_cs, flag);
    idt_install(IDT_AC,  asm_alignfault,    kern_cs, flag);
    idt_install(IDT_MC,  asm_machine_check, kern_cs, flag);
    idt_install(IDT_XF,  asm_simdfault,     kern_cs, flag);
    return 0;
}

/**
 * Install all system calls
 * @return 0 as success
 */
int syscall_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_USER;

    idt_install(FORK_INT,           asm_fork,           kern_cs, flag);
    idt_install(EXEC_INT,           asm_exec,           kern_cs, flag);
    idt_install(WAIT_INT,           asm_wait,           kern_cs, flag);
    idt_install(YIELD_INT,          asm_yield,          kern_cs, flag);
    idt_install(DESCHEDULE_INT,     asm_deschedule,     kern_cs, flag);
    idt_install(MAKE_RUNNABLE_INT,  asm_make_runnable,  kern_cs, flag);
    idt_install(GETTID_INT,         asm_gettid,         kern_cs, flag);
    idt_install(NEW_PAGES_INT,      asm_new_pages,      kern_cs, flag);
    idt_install(REMOVE_PAGES_INT,   asm_remove_pages,   kern_cs, flag);
    idt_install(SLEEP_INT,          asm_sleep,          kern_cs, flag);
    idt_install(READLINE_INT,       asm_readline,       kern_cs, flag);
    idt_install(PRINT_INT,          asm_print,          kern_cs, flag);
    idt_install(SET_TERM_COLOR_INT, asm_set_term_color, kern_cs, flag);
    idt_install(SET_CURSOR_POS_INT, asm_set_cursor_pos, kern_cs, flag);
    idt_install(GET_CURSOR_POS_INT, asm_get_cursor_pos, kern_cs, flag);
    idt_install(THREAD_FORK_INT,    asm_thread_fork,    kern_cs, flag);
    idt_install(GET_TICKS_INT,      asm_get_ticks,      kern_cs, flag);
    idt_install(HALT_INT,           asm_halt,           kern_cs, flag);
    idt_install(SET_STATUS_INT,     asm_set_status,     kern_cs, flag);
    idt_install(VANISH_INT,         asm_vanish,         kern_cs, flag);
    idt_install(SWEXN_INT,          asm_swexn,          kern_cs, flag);
    return 0;
}

/**
 * Install keyboard driver and timer driver.
 * @return 0 as success
 */
int device_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_KERNEL;
    timer_init(timer_callback);
    idt_install(TIMER_IDT_ENTRY, asm_timer_handler,    kern_cs, flag);
    idt_install(KEY_IDT_ENTRY,   asm_keyboard_handler, kern_cs, flag);
    return 0;
}

/**
 * Install handler to IDT table and set flags
 * @idt_idx     index of each IDT entry
 * @entry       entry address of the handler
 * @selector    code selector
 * @flag.       IDT flags
 */
void idt_install(int idt_idx, void (*entry)(), int selector, int flag) {
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
