/** @file handlers.c
 *  @author Newton Xie (ncx)
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

extern uint32_t *kern_page_dir;
extern uint32_t zfod_frame;

void pf_handler() {
    uint32_t pf_addr = get_cr2();
    uint32_t pte = get_pte(pf_addr);

    // TODO handle other cases
    if (!(pte & PTE_PRESENT) || (pte & PAGE_ALIGN_MASK) == zfod_frame) {
        uint32_t frame_addr = get_frame();
        set_pte(pf_addr, frame_addr, PTE_WRITE | PTE_USER | PTE_PRESENT);
    } else {
        lprintf("%x\n", (unsigned int)pf_addr);
        roll_over();
    }

    return;
}

/** @brief
 *
 *  @param
 *  @return
 */
int handler_init() {
    RETURN_IF_ERROR(exception_init(), ERROR_TRAP_INSTALL_FAILED);
    RETURN_IF_ERROR(syscall_init(), ERROR_SYSCALL_INSTALL_FAILED);
    RETURN_IF_ERROR(device_init(), ERROR_DEVICE_INSTALL_FAILED);

    return SUCCESS;
}

int exception_init() {
    idt_install(IDT_PF,
                asm_pf_handler,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_KERNEL);
    return SUCCESS;
}

int syscall_init() {
    idt_install(FORK_INT,
                (void *)asm_fork,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(EXEC_INT,
                (void *)asm_exec,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(WAIT_INT,
                (void *)asm_wait,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(YIELD_INT,
                (void *)asm_yield,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(DESCHEDULE_INT,
                (void *)asm_deschedule,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(MAKE_RUNNABLE_INT,
                (void *)asm_make_runnable,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(GETTID_INT,
                (void *)asm_gettid,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(NEW_PAGES_INT,
                (void *)asm_new_pages,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(READLINE_INT,
                (void *)asm_readline,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(SET_STATUS_INT,
                (void *)asm_set_status,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(VANISH_INT,
                (void *)asm_vanish,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);

    return SUCCESS;
}

int device_init() {
    timer_init(timer_callback);
    idt_install(TIMER_IDT_ENTRY,
                (void *)asm_timer_handler,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_KERNEL);
    idt_install(KEY_IDT_ENTRY,
                (void *)asm_keyboard_handler,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_KERNEL);
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
