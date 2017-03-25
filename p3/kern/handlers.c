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
#include "asm_handlers.h"
#include "vm.h"
#include "task.h"
#include "asm_timer_handler.h"
#include "asm_keyboard_handler.h"   /* asm_keyboard_handler */
#include "timer_driver.h"
#include "syscalls.h"
#include "asm_syscalls.h"
#include "return_type.h"            /* RETURN_IF_ERROR, ERROR_TYPE */

extern uint32_t *kern_page_dir;

void pf_handler() {
    uint32_t pf_addr = get_cr2();
    uint32_t pte = get_pte(pf_addr);

    if (!(pte & PTE_PRESENT)) {
        uint32_t frame = get_free_frame();
        int flags = PTE_WRITE | PTE_USER;
        set_pte(pf_addr, frame, flags);
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

    RETURN_IF_ERROR(trap_init(), ERROR_TRAP_INSTALL_FAILED);
    RETURN_IF_ERROR(syscall_init(), ERROR_SYSCALL_INSTALL_FAILED);
    RETURN_IF_ERROR(device_init(), ERROR_DEVICE_INSTALL_FAILED);

    return SUCCESS;
}

int trap_init() {
    idt_install(IDT_PF,
                asm_pf_handler,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_KERNEL);
    return SUCCESS;
}

int syscall_init() {
    idt_install(EXEC_INT,
                (void *)asm_exec,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    idt_install(GETTID_INT,
                (void *)asm_gettid,
                SEGSEL_KERNEL_CS,
                FLAG_TRAP_GATE | FLAG_PL_USER);
    return SUCCESS;
}

int device_init() {
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
                 unsigned int selector,
                 unsigned int flag) {
    uint32_t *idt_addr = (uint32_t *)idt_base() + 2 * idt_idx;
    *idt_addr = ((uint32_t)entry & 0x0000ffff) | (selector << 16);
    *(idt_addr + 1) =
        ((uint32_t)entry & 0xffff0000) | ((flag | FLAG_PRESENT) << 8);
}

void roll_over() {
    lprintf("feeLs bad man\n");
    while (1) continue;
}
