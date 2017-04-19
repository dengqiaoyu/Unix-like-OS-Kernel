/**
 * @file   exceptions.c
 * @brief  This file contains functions that are used to handle exceptions or
 *         call user defined software handler.
 * @author Newton Xie (ncw)
 * @author Qiaoyu Deng (qdeng)
 * @bug No known bugs
 */

/* libc includes */
#include <stdlib.h>
#include <string.h>                     /* memcpy */
#include <ureg.h>                       /* ureg_t */
#include <stdio.h>                      /* printf */

/* x86 specific includes */
#include <x86/cr.h>                     /* get_cr2 */
#include <x86/idt.h>                    /* IDT */

/* DEBUG */
#include <simics.h>                     /* lprintf */

#include "exceptions/exceptions.h"
#include "vm.h"                         /* get_zfod_frame */
#include "task.h"                       /* thread_t */
#include "scheduler.h"                  /* get_cur_tcb */
#include "asm_page_inval.h"             /* asm_page_inval */
#include "syscalls/syscalls.h"          /* kern_halt, kern_vanish */

/* internal functions */
/**
 * @brief      print error message about fault type and where the error
 *             happens(eip), and esp when the error has error code, and also
 *             print memory address when page fault happens, and dump all the
 *             registers' value.
 * @param ureg structs that contains all error information
 */
static void _exn_print_error_msg(ureg_t *ureg);

/**
 * @brief      called by _exn_print_error_msg, dump all the registers'
 *             information
 * @param ureg structs that contains all error information
 */
static void _exn_print_ureg(ureg_t *ureg);

/**
 * @brief   handle with PAGEFAULT fault, when it is a ZFOD frame just assign a
 *          new frame to the page fault address
 */
void pagefault_handler() {
    /* get the page fault address from cr2 */
    uint32_t pf_addr = get_cr2();
    /* get which page table entry this address belongs to  */
    uint32_t pte = get_pte(pf_addr);

    if ((pte & PAGE_ALIGN_MASK) == get_zfod_frame()) {
        /* we need to invalidate this address in TLB because we put new frame */
        asm_page_inval((void *)pf_addr);
        uint32_t frame_addr = get_frame();
        set_pte(pf_addr, frame_addr, PTE_WRITE | PTE_USER | PTE_PRESENT);
    } else {
        /* otherwise call handler to handle page fault */
        exn_handler(SWEXN_CAUSE_PAGEFAULT, ERROR_CODE);
    }
}

/**
 * @brief         Handle hardware exceptions that are impossible to recover by
 *                just halting the entire system.
 * @param cause   cause code that are defined in ureg.h or idt.h
 * @param ec_flag whether this error has error code
 */
void hwerror_handler(int cause, int ec_flag) {
    /* IDT_CSO, IDT_TS, IDT_MC */
    switch (cause) {
    case IDT_CSO:
        printf("Coprocessor Segment Overrun\n");
        break;
    case IDT_TS:
        printf("Invalid TSS\n");
        break;
    case IDT_MC:
        printf("Machine Check Error\n");
        break;
    default:
        break;
    }
    kern_halt();
}

// TODO
/**
 * @brief         Global exception handler for calling default handler or
 *                software exception handler.
 * @param cause   cause code that are defined in ureg.h or idt.h
 * @param ec_flag whether this error has error code
 */
void exn_handler(int cause, int ec_flag) {
    thread_t *thread = get_cur_tcb();

    uint32_t *esp0 = (uint32_t *)(thread->kern_sp);
    uint32_t *esp3;
    ureg_t ureg;
    if (thread->swexn_handler == NULL) {
        esp3 = (uint32_t *)(&ureg);
        esp3 += sizeof(ureg_t) / sizeof(uint32_t);
    } else {
        esp3 = thread->swexn_sp;
    }

    // begin setting up ureg

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

    // ureg is set up
    if (thread->swexn_handler == NULL) {
        task_t *task = thread->task;
        kern_mutex_lock(&(task->thread_list_mutex));
        int live_threads = get_list_size(task->live_thread_list);
        if (live_threads == 1) task->status = -2;
        kern_mutex_unlock(&(task->thread_list_mutex));
        _exn_print_error_msg(&ureg);
        kern_vanish();
    } else {
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
}

/**
 * @brief      print error message about fault type and where the error
 *             happens(eip), and esp when the error has error code, and also
 *             print memory address when page fault happens, and dump all the
 *             registers' value.
 * @param ureg structs that contains all error information
 */
void _exn_print_error_msg(ureg_t *ureg) {
    unsigned int fault_addr = ureg->cr2;
    unsigned int fault_ip_addr = ureg->eip;
    int error_code = ureg->error_code;
    printf("Instruction at 0x%08x accessing 0x%08x got exception\n",
           fault_ip_addr, fault_addr);
    switch (ureg->cause) {
    case SWEXN_CAUSE_DIVIDE:
        printf("DIVIDE ERROR\n");
        break;
    case SWEXN_CAUSE_DEBUG:
        printf("DEBUG ERROR\n");
        break;
    case SWEXN_CAUSE_BREAKPOINT:
        printf("BREAKPOINT ERROR\n");
        break;
    case SWEXN_CAUSE_OVERFLOW:
        printf("OVERFLOW ERROR\n");
        break;
    case SWEXN_CAUSE_BOUNDCHECK:
        printf("BOUNDCHECK ERROR\n");
        break;
    case SWEXN_CAUSE_OPCODE:
        printf("OPCODE ERROR\n");
        break;
    case SWEXN_CAUSE_NOFPU:
        printf("NOFPU ERROR\n");
        break;
    case SWEXN_CAUSE_SEGFAULT:
        printf("SEGFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_STACKFAULT:
        printf("STACKFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_PROTFAULT:
        printf("PROTFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_PAGEFAULT:
        printf("PAGEFAULT ERROR:\n");
        if (error_code & ERROR_CODE_P) printf("non-present page's");
        else printf("non-present page's");
        if (error_code & ERROR_CODE_WR) printf(" write fault");
        else printf(" read fault");
        if (error_code & ERROR_CODE_US) printf(" in user mode");
        else printf(" in supervisor mode");
        if (error_code & ERROR_CODE_RSVD)
            printf(" with reserved bit violation\n");
        else printf("\n");
        break;
    case SWEXN_CAUSE_FPUFAULT:
        printf("FPUFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_ALIGNFAULT:
        printf("ALIGNFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_SIMDFAULT:
        printf("SIMDFAULT ERROR\n");
        break;
    default:
        printf("UNKNOWN ERROR\n");
    }
    _exn_print_ureg(ureg);
}

/**
 * @brief      called by _exn_print_error_msg, dump all the registers'
 *             information
 * @param ureg structs that contains all error information
 */
void _exn_print_ureg(ureg_t *ureg) {
    printf("Registers Dump:\n");
    printf("cause: %d, error_code: %d, cr2: 0x%08x\n",
           ureg->cause, ureg->error_code, ureg->cr2);
    printf("cs:  0x%08x, ss:  0x%08x\n", ureg->cs, ureg->ss);
    printf("ds:  0x%08x, es:  0x%08x, fs:  0x%08x, gs:  0x%08x\n",
           ureg->ds, ureg->es, ureg->fs, ureg-> gs);
    printf("edi: 0x%08x, esi: 0x%08x, ebp: 0x%08x\n",
           ureg->edi, ureg->esi, ureg->ebp);
    printf("eax: 0x%08x, ebx: 0x%08x, ecx: 0x%08x, edx: 0x%08x\n",
           ureg->eax, ureg->ebx, ureg->ecx, ureg->edx);
    printf("eip: 0x%08x, esp: 0x%08x\neflags: 0x%08x\n\n",
           ureg->eip, ureg->esp, ureg->eflags);
}