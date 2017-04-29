/** @file hypervisor.c
 */

#include <stdlib.h>
#include <x86/seg.h>
#include <common_kern.h>         /* USER_MEM_START */
#include <cr.h>                  /* set_crX */
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */
#include <console.h>             /* clear_console */
#include <simics.h>              /* lprintf and debug */

#include "task.h"
#include "vm.h"                 /* predefines for virtual memory */
#include "scheduler.h"          /* schedule node */
#include "inc/hypervisor.h"
#include "inc/asm_kern_to_user.h"

extern uint64_t init_gdt[GDT_SEGS];

void hypervisor_init() {
    init_gdt[SEGSEL_SPARE0_IDX] = SEGDES_GUEST_CS;
    init_gdt[SEGSEL_SPARE1_IDX] = SEGDES_GUEST_DS;
}

int guest_init(simple_elf_t *header) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    uint32_t addr = USER_MEM_START;
    int pte_flags = PTE_USER | PTE_WRITE | PTE_PRESENT;

    if (dec_num_free_frames(GUEST_FRAMES) < 0) return -1;
    int i;
    for (i = 0; i < GUEST_FRAMES; i++) {
        uint32_t frame_addr = get_frame();
        if (set_pte(addr, frame_addr, pte_flags) < 0) {
            inc_num_free_frames(GUEST_FRAMES);
            // task of freeing pages is deferred to caller
            return -1;
        }
        addr += PAGE_SIZE;
    }

    int ret = 0;

    /* load text */
    ret = load_guest_section(header->e_fname,
                             header->e_txtstart,
                             header->e_txtlen,
                             header->e_txtoff);
    if (ret < 0) return -1;

    /* load dat */
    ret = load_guest_section(header->e_fname,
                             header->e_datstart,
                             header->e_datlen,
                             header->e_datoff);
    if (ret < 0) return -1;

    /* load rodat */
    ret = load_guest_section(header->e_fname,
                             header->e_rodatstart,
                             header->e_rodatlen,
                             header->e_rodatoff);
    if (ret < 0) return -1;

    // update fname for simics symbolic debugging
    sim_reg_process(task->page_dir, header->e_fname);

    set_esp0(thread->kern_sp);
    kern_to_guest(header->e_entry);

    return 0;
}

int load_guest_section(const char *fname, unsigned long start,
                       unsigned long len, unsigned long offset) {
    uint32_t high = (uint32_t)(start + len);
    if (high >= GUEST_MEM_SIZE) return -1;

    char *linear_start = (char *)start + USER_MEM_START;
    getbytes(fname, offset, len, linear_start);
    return 0;
}

