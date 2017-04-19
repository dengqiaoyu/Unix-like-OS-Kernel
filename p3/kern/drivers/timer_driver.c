/**
 *  @file time_driver.c
 *  @brief A time driver that can conut time for every 10ms.
 *  @author Qiaoyu Deng
 *  @andrew_id qdeng
 *  @bug No known bugs.
 */

#include <x86/timer_defines.h>

/* x86 includes */
#include <asm.h>                /* outb() */
#include <interrupt_defines.h>  /* INT_ACK_CURRENT, INT_CTL_PORT */
#include <stdio.h>              /* NULL */

/* debug includes */
#include <simics.h>             /* lprintf() */

/* user defined includes */
#include "inc/drivers/timer_driver.h"
#include "scheduler.h"

static int num_ticks;
static void (*callback_func)(); /* to store the address of callback function */

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
    callback_func(num_ticks);
}

int get_timer_ticks() {
    return num_ticks;
}
