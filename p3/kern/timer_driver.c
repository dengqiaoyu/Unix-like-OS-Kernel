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
#include "inc/timer_driver.h"
#include "scheduler.h"

int num_ticks;
static int seconds = 0;
int cnt_seconds_flag = 0; /* control whether to count seconds */
void (*callback_func)(); /* to store the address of callback function */

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
    callback_func(num_ticks);
    // outb(INT_ACK_CURRENT, INT_CTL_PORT);
}

/**
 * @brief Callback function that is called by the handler.
 * @param num_ticks the number of 10 ms that is triggered.
 */
void timer_callback(unsigned int num_ticks) {
    if (cnt_seconds_flag && (num_ticks % 100 == 0)) {
        // lprintf("time: %d", seconds);
        seconds++;
    }
    // outb(INT_ACK_CURRENT, INT_CTL_PORT);
    sche_yield(RUNNABLE);
}

/**
 * @brief get current time.
 * @return current time.
 */
int get_seconds() {
    return seconds;
}
