/**
 * @file thread_management.c
 * @brief This file contains thread management system calls for controlling
 *        thread running status.
 * @author Newton Xie(ncx)
 * @author Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

/* libc includes */
#include <stdlib.h>
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */
#include <x86/eflags.h>
#include <x86/seg.h>
#include <string.h>

/* DEBUG */
#include <simics.h>

#include "exceptions/exceptions.h"
#include "syscalls/syscalls.h"
#include "task.h"                 /* task thread declaration and interface */
#include "scheduler.h"            /* scheduler declaration and interface */
#include "utils/tcb_hashtab.h"    /* insert and find tcb by tid */
#include "drivers/timer_driver.h" /* get_timer_ticks */

/**
 * @brief Get thread id
 * @return  tid
 */
int kern_gettid(void) {
    thread_t *thread = get_cur_tcb();
    return thread->tid;
}

/**
 * @brief Yield to a thread whose tid is indicated by the argument, if tid is -1
 *        just yield to the next thread in the scheduler.
 * @return  0 when yield succeeds, -1 when the tid does not exist or the target
 *          thread is not runnable return -1
 */
int kern_yield(void) {
    /* tid is stored in %esi */
    int tid = (int)asm_get_esi();
    if (tid < -1) return -1;

    if (tid == -1) {
        sche_yield(RUNNABLE);
        return 0;
    }

    /**
     * We need disable interrupts here, because we need to ensure no one can
     * change thread's status before we actually yield to that thread.
     */
    disable_interrupts();
    thread_t *thr = tcb_hashtab_get(tid);
    if (thr == NULL) return -1;
    if (thr->status != RUNNABLE) {
        enable_interrupts();
        return -1;
    }
    sche_move_front(thr);
    sche_yield(RUNNABLE);

    return 0;
}

/**
 * @brief   Atomically checks the integer pointed to by reject and acts on it.
 *          If the integer is non-zero, the call returns immediately with return
 *          value zero. If the integer pointed to by reject is zero, then the
 *          calling thread will not be run by the scheduler until a make
 *          runnable() call is made specifying the deschedule()’s thread
 * @return  0 when the value pointed by reject is non-zero, or descheduled
 *          successfully. Otherwise return -1
 */
int kern_deschedule(void) {
    int *reject = (int *)asm_get_esi();

    /* still need to ensure no one can change reject's addressed value */
    disable_interrupts();
    if (*reject != 0) {
        enable_interrupts();
        return 0;
    }
    sche_yield(SUSPENDED);

    return 0;
}

/**
 * @brief   Makes the deschedule()d thread with ID tid runnable by the scheduler
 * @return  0 as success , -1 when thread does not exist or thread is currently
 *          non-runnable due to a call to deschedule()
 */
int kern_make_runnable(void) {
    int tid = (int)asm_get_esi();

    disable_interrupts();
    thread_t *thr = tcb_hashtab_get(tid);
    if (thr == NULL) return -1;
    if (thr->status != SUSPENDED) return -1;
    thr->status = RUNNABLE;
    sche_push_front(thr);
    enable_interrupts();

    return 0;
}

/**
 * @brief   Returns the number of timer ticks which have occurred since system
 *          boot.
 * @return  ticks value
 */
unsigned int kern_get_ticks(void) {
    return get_timer_ticks();
}

/**
 * @brief   Deschedules the calling thread until at least ticks timer
 *          interrupts have occurred after the call.
 * @return  return immediately if ticks is zero, return -1 if ticks is a
 *          negative value, return on success
 */
int kern_sleep(void) {
    int ticks = (int)asm_get_esi();

    if (ticks == 0) return 0;
    unsigned int cur_ticks = get_timer_ticks();
    unsigned int wakeup_ticks = cur_ticks + (unsigned int)ticks;
    if (wakeup_ticks < cur_ticks) return -1;

    sleep_node_t sleep_node;
    sleep_node.thread = get_cur_tcb();
    sleep_node.wakeup_ticks = wakeup_ticks;

    disable_interrupts();
    tranquilize(&sleep_node);
    sche_yield(SLEEPING);

    return 0;
}

/** @brief Installs a swexn handler.
 *
 *  If esp3 or eip are zero, the current handler is deregistered if one exists.
 *  Otherwise, a handler is registered, overwriting any current handler. If
 *  newureg is non-null, then the system attempts to set all registers to the
 *  newureg values before returning from kern_swexn. This is done by copying
 *  newureg attributes to the stack. These stack values will be popped off
 *  by the return sequence in asm_swexn.
 *
 *  If any of the parameters are judged to be invalid, no actions are taken.
 * 
 *  @return 0 on success and negative on failure (may not return)
 */
int kern_swexn(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    void *esp3 = (void *)(*esi);
    swexn_handler_t eip = (swexn_handler_t)(*(esi + 1));
    void *arg = (void *)(*(esi + 2));
    ureg_t *newureg = (ureg_t *)(*(esi + 3));

    thread_t *thread = get_cur_tcb();

    if (newureg != NULL) {
        uint32_t zero = 0;
        // check that io privilege is not changed
        zero |= (newureg->eflags & EFL_IOPL_RING3);
        // check that these pairs are all equal
        zero |= (newureg->cs ^ SEGSEL_USER_CS);
        zero |= (newureg->ss ^ SEGSEL_USER_DS);
        zero |= (newureg->ds ^ SEGSEL_USER_DS);
        zero |= (newureg->es ^ SEGSEL_USER_DS);
        zero |= (newureg->fs ^ SEGSEL_USER_DS);
        zero |= (newureg->gs ^ SEGSEL_USER_DS);
        if (zero != 0) return -1;
        // check that interrupts are enabled
        if (!(newureg->eflags & EFL_IF)) return -1;
    }

    if (esp3 == NULL || eip == NULL) {
        thread->swexn_sp = NULL;
        thread->swexn_handler = NULL;
        thread->swexn_arg = NULL;
    } else {
        int ret;
        
        /* store the required size of esp3 for validation */
        uint32_t esp3_size = sizeof(ureg_t);
        /* stack must also store its two arguments and a dummy return address */
        esp3_size += 3 * sizeof(int);

        uint32_t esp3_low = (uint32_t)esp3 - esp3_size;
        ret = validate_user_mem(esp3_low, esp3_size, MAP_USER | MAP_WRITE);
        if (ret < 0) return -1;
        /* check that eip is in executable user code */
        ret = validate_user_mem((uint32_t)eip, 1, MAP_USER | MAP_EXECUTE);
        if (ret < 0) return -1;

        /* parameters seem valid, so install the handler */
        thread->swexn_sp = esp3;
        thread->swexn_handler = eip;
        thread->swexn_arg = arg;
    }

    if (newureg != NULL) {
        /* copy over stack values to appropriate ureg_t positions */
        int stack_regs_offset = sizeof(int) * (N_IRET_PARAMS + N_REGISTERS);
        int stack_regs_size = sizeof(int) * N_REGISTERS;
        uint32_t stack_regs_ptr = thread->kern_sp - stack_regs_offset;
        memcpy((void *)stack_regs_ptr, &(newureg->edi), stack_regs_size);

        int iret_args_offset = sizeof(int) * N_IRET_PARAMS;
        uint32_t iret_args_ptr = thread->kern_sp - iret_args_offset;
        memcpy((void *)iret_args_ptr, &(newureg->eip), iret_args_offset);

        return 1;
    }

    return 0;
}
