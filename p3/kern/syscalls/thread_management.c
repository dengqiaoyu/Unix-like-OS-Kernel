#include <stdlib.h>
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */
#include <simics.h>
#include <x86/eflags.h>
#include <x86/seg.h>
#include <string.h>

#include "syscalls/syscalls.h"
#include "task.h"                /* task thread declaration and interface */
#include "scheduler.h"           /* scheduler declaration and interface */
#include "utils/tcb_hashtab.h"         /* insert and find tcb by tid */
#include "drivers/timer_driver.h"        /* get_timer_ticks */

int kern_gettid(void) {
    thread_t *thread = get_cur_tcb();
    return thread->tid;
}

int kern_yield(void) {
    int tid = (int)asm_get_esi();

    if (tid < -1) return -1;
    if (tid == -1) {
        sche_yield(RUNNABLE);
        return 0;
    }
    thread_t *thr = tcb_hashtab_get(tid);
    if (thr == NULL) return -1;
    disable_interrupts();
    if (thr->status != RUNNABLE) {
        enable_interrupts();
        return -1;
    }
    sche_push_front(thr);
    sche_yield(RUNNABLE);
    enable_interrupts();
    return 0;
}

int kern_deschedule(void) {
    int *reject = (int *)asm_get_esi();

    disable_interrupts();
    if (*reject != 0) {
        enable_interrupts();
        return 0;
    }
    sche_yield(SUSPENDED);
    enable_interrupts();
    return 0;
}

int kern_make_runnable(void) {
    int tid = (int)asm_get_esi();

    thread_t *thr = tcb_hashtab_get(tid);
    if (thr == NULL) return -1;
    if (thr->status != SUSPENDED) return -1;
    disable_interrupts();
    thr->status = RUNNABLE;
    sche_push_back(thr);
    enable_interrupts();
    return 0;
}

unsigned int kern_get_ticks(void) {
    return get_timer_ticks();
}

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
    }
    else {
        int ret;
        // TODO macro
        // 3 because ret addr and two args
        uint32_t esp3_size = sizeof(ureg_t) + 3 * sizeof(int);
        uint32_t esp3_low = (uint32_t)esp3 - esp3_size;
        ret = validate_user_mem(esp3_low, esp3_size, MAP_USER | MAP_WRITE);
        if (ret < 0) return -1;
        ret = validate_user_mem((uint32_t)eip, 1, MAP_USER | MAP_EXECUTE);
        if (ret < 0) return -1;

        thread->swexn_sp = esp3;
        thread->swexn_handler = eip;
        thread->swexn_arg = arg;
    }

    if (newureg != NULL) {
        // TODO macro
        int stack_regs_offset = sizeof(int) * (5 + 8);
        int stack_regs_size = sizeof(int) * 8;
        uint32_t stack_regs_ptr = thread->kern_sp - stack_regs_offset;
        memcpy((void *)stack_regs_ptr, &(newureg->edi), stack_regs_size);

        int iret_args_offset = sizeof(int) * 5;
        uint32_t iret_args_ptr = thread->kern_sp - iret_args_offset;
        memcpy((void *)iret_args_ptr, &(newureg->eip), iret_args_offset);

        return 1;
    }

    return 0;
}
