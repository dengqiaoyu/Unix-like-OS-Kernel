#include <stdlib.h>
#include <string.h>
#include <simics.h>
#include <ureg.h>
#include <x86/seg.h>
#include <x86/cr.h>
#include <x86/asm.h>
/*
#include <x86/asm.h>
#include <x86/idt.h>
*/

#include "vm.h"
#include "task.h"
#include "scheduler.h"
#include "syscalls/life_cycle.h"
#include "exceptions/exceptions.h"
#include "asm_page_inval.h"

void pagefault_handler() {
    uint32_t pf_addr = get_cr2();
    uint32_t pte = get_pte(pf_addr);

    if ((pte & PAGE_ALIGN_MASK) == get_zfod_frame()) {
        asm_page_inval((void *)pf_addr);
        uint32_t frame_addr = get_frame();
        set_pte(pf_addr, frame_addr, PTE_WRITE | PTE_USER | PTE_PRESENT);
    } else {
        exn_handler(SWEXN_CAUSE_PAGEFAULT, ERROR_CODE);
    }
}

void exn_handler(int cause, int ec_flag) {
    thread_t *thread = get_cur_tcb();

    if (thread->swexn_handler == NULL) {
        task_t *task = thread->task;
        task->status = -2;
        kern_vanish();
        return;
    }

    uint32_t *esp0 = (uint32_t *)(thread->kern_sp);
    uint32_t *esp3 = thread->swexn_sp;

    // TODO macro
    esp0 -= 5;
    esp3 -= 5;
    memcpy(esp3, esp0, sizeof(int) * 5);

    esp0 -= (8 + ec_flag);
    esp3 -= (8 + 1);
    memcpy(esp3, esp0, sizeof(int) * (8 + 1));
    if (!ec_flag) *(esp3 + 8) = 0;

    esp0 -= 4;
    esp3 -= 4;
    memcpy(esp3, esp0, sizeof(int) * 4);

    if (cause == SWEXN_CAUSE_PAGEFAULT) *(--esp3) = get_cr2();
    else *(--esp3) = 0;

    *(--esp3) = cause;
    uint32_t ureg_ptr = (uint32_t)esp3;
    *(--esp3) = ureg_ptr;
    uint32_t swexn_arg = (uint32_t)thread->swexn_arg;
    *(--esp3) = swexn_arg;
    *(--esp3) = 0;

    esp0 = (uint32_t *)(thread->kern_sp);
    *(esp0 - 2) = (uint32_t)esp3;
    *(esp0 - 5) = (uint32_t)(thread->swexn_handler);

    thread->swexn_sp = NULL;
    thread->swexn_handler = NULL;
    thread->swexn_arg = NULL;
}
