/** @file handlers.c
 *  @author Newton Xie (ncx) Qiaoyu Deng(qdeng)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <simics.h>
#include <x86/asm.h>
#include <x86/seg.h>
#include <x86/page.h>
#include <x86/cr.h>
#include <x86/idt.h>
#include <syscall_int.h>
#include <x86/timer_defines.h>  /* TIMER_IDT_ENTRY */
#include <keyhelp.h>            /* KEY_IDT_ENTRY */

#include "handlers.h"
#include "vm.h"
#include "task.h"
#include "syscalls.h"
#include "asm_syscalls.h"
#include "asm_exceptions.h"
#include "asm_interrupts.h"
#include "timer_driver.h"
#include "keyboard_driver.h"
#include "return_type.h"            /* RETURN_IF_ERROR, ERROR_TYPE */
#include "asm_page_inval.h"

extern uint32_t *kern_page_dir;
extern uint32_t zfod_frame;

void pf_handler() {
    uint32_t pf_addr = get_cr2();
    uint32_t pte = get_pte(pf_addr);

    // TODO handle other cases
    if ((pte & PAGE_ALIGN_MASK) == zfod_frame) {
        page_inval((void *)pf_addr);
        uint32_t frame_addr = get_frame();
        set_pte(pf_addr, frame_addr, PTE_WRITE | PTE_USER | PTE_PRESENT);
    } else if (!(pte & PTE_PRESENT)) {
        lprintf("alarming...");
        MAGIC_BREAK;
        uint32_t frame_addr = get_frame();
        set_pte(pf_addr, frame_addr, PTE_WRITE | PTE_USER | PTE_PRESENT);
    } else {
        lprintf("%x\n", (unsigned int)pf_addr);
        roll_over();
    }

    return;
}

int handler_init() {
    exception_init();
    syscall_init();
    device_init();
    return SUCCESS;
}

int exception_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_KERNEL;
    idt_install(IDT_PF, asm_pf_handler, kern_cs, flag);
    return SUCCESS;
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
    // idt_install(SWEXN_INT,       (void *)asm_swexn,          kern_cs, flag);
    return SUCCESS;
}

int device_init() {
    int kern_cs = SEGSEL_KERNEL_CS;
    int flag = FLAG_TRAP_GATE | FLAG_PL_KERNEL;
    timer_init(timer_callback);
    idt_install(TIMER_IDT_ENTRY, (void *)asm_timer_handler,    kern_cs, flag);
    idt_install(KEY_IDT_ENTRY,   (void *)asm_keyboard_handler, kern_cs, flag);
    return SUCCESS;
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

void roll_over() {
    lprintf("feeLs bad man\n");
    while (1) continue;
}
