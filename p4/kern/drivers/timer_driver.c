/**
 *  @file    time_driver.c
 *  @brief   A time driver that can conut time for every 10ms.
 *  @author  Qiaoyu Deng (qdeng)
 *  @bug     No known bugs.
 */

#include <x86/timer_defines.h>

/* x86 specific includes */
#include <asm.h>                /* outb() */
#include <interrupt_defines.h>  /* INT_ACK_CURRENT, INT_CTL_PORT */
#include <stdio.h>              /* NULL */

/* debug includes */
#include <simics.h>             /* lprintf() */

/* user defined includes */
#include "drivers/timer_driver.h"
#include "scheduler.h"

static int num_ticks;
static void (*callback_func)(); /* to store the address of callback function */

extern guest_info_t *guest_info_driver;

static void _prepare_guest_timer(void);
/**
 * @brief Initialize timer.
 * @param tickback the address of call back function.
 */
void timer_init(void (*tickback)(unsigned int)) {
    unsigned short cycles_per_interrupt =
        (TIMER_RATE / (MS_PER_S / MS_PER_INTERRUPT));
    /* set to zero */
    outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
    /* set the frequency */
    outb(TIMER_PERIOD_IO_PORT, LSB(cycles_per_interrupt));
    outb(TIMER_PERIOD_IO_PORT, MSB(cycles_per_interrupt));
    /* initiallize callback function */
    num_ticks = 0;
    callback_func = tickback;
}

/**
 * @brief Increment num_ticks per 10 ms and set callback function.
 */
void timer_handler() {
    num_ticks++;
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
    _prepare_guest_timer();
    callback_func(num_ticks);
}

void _prepare_guest_timer(void) {
    thread_t *thread = get_cur_tcb();
    if (thread == NULL) return;
    guest_info_t *guest_info = thread->task->guest_info;
    if (guest_info == NULL || guest_info->timer_init_stat != TIMER_INTED)
        return;
    uint32_t guest_interval = guest_info->timer_interval;
    uint32_t host_interval = MS_PER_INTERRUPT;
    uint32_t multiple = guest_interval / host_interval;
    guest_info->internal_ticks++;
    if (guest_info->internal_ticks % multiple == 0)
        set_user_handler(TIMER_DEVICE);
    return;
}

int get_timer_ticks() {
    return num_ticks;
}
