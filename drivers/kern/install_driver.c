/**
 * @brief This file contain the functions that is used to install timer and
 *        keyboard driver to the IDT table.
 * @author Qiaoyu Deng
 * @andrew_id qdeng
 * @bug No known bug.
 */

#include <p1kern.h>

#include <seg.h>                    /* for SEGSEL_KERNEL_CS */
#include <timer_defines.h>          /* TIMER_IDT_ENTRY */
#include <keyhelp.h>                /* KEY_IDT_ENTRY */
#include <asm.h>                    /* idt_base() */
#include <string.h>                 /* memcpy() */

#include <simics.h>                 /* lprintf() */

#include "inc/timer_driver.h"       /* init_timer(), tickback() */
#include "inc/timer_handler.h"      /* timer_handler */
#include "inc/keyboard_handler.h"   /* keyboard_handler */
#include "inc/install_driver.h"

/* function definition */
/**
 * @brief Install the timer driver and keyboard driver.
 * @param tickback the address of callback function.
 */
int handler_install(void (*tickback)(unsigned int)) {
    init_timer(tickback);
    add_to_idt_gate(TIMER_IDT_ENTRY, timer_handler);
    add_to_idt_gate(KEY_IDT_ENTRY, keyboard_handler);
    return 0;
}

/**
 * @brief Add trap gate to the corresponding IDT entery.
 * @param idt_gate_index the index of the corresponding IDT entery.
 * @param handler_addr   the address of interrupt handler.
 */
void add_to_idt_gate(int idt_gate_index, void *handler_addr) {
    /* disable the interrpt while we are installing some part of it */
    disable_interrupts();
    void *idt_base_addr = idt_base();
    struct idt_gates idt_gate = {0};
    fill_gate(&idt_gate, handler_addr);

    void *idt_entry = idt_base_addr + idt_gate_index * IDT_ENTRY_LENGTH;
    memcpy(idt_entry, &idt_gate, sizeof(struct idt_gates));
    enable_interrupts();
}

/**
 * @brief Fill data (like handler address) to the gate entry.
 * @param idt_gate     the pointer to the trap gate structure.
 * @param handler_addr the address of the handler.
 */
void fill_gate(struct idt_gates *idt_gate, void *handler_addr) {
    idt_gate->lsb_offset = (unsigned int)handler_addr;
    idt_gate->seg_sel = SEGSEL_KERNEL_CS;
    idt_gate->reserved = RESERVED;
    idt_gate->zeros = ZEROS;
    idt_gate->gate_type = GATE_TYPE;
    idt_gate->d = D;
    idt_gate->zero = 0;
    idt_gate->dpl = DPL;
    idt_gate->p = P;
    idt_gate->msb_offset = (unsigned int)handler_addr >> 16;
}
