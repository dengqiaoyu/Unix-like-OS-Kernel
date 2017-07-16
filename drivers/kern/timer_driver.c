/**
 *  @file time_driver.c
 *  @brief A time driver that can conut time for every 10ms.
 *  @author Qiaoyu Deng
 *  @andrew_id qdeng
 *  @bug No known bugs.
 */

// #include <timer_defines.h>

/* x86 includes */
#include <asm.h>                /* outb() */
#include <interrupt_defines.h>  /* INT_ACK_CURRENT, INT_CTL_PORT */

/* debug includes */
#include <simics.h>             /* lprintf() */

/* user defined includes */
#include "inc/timer_driver.h"


static int numTicks;
static int seconds = 0;
/* control whether to count seconds */
int if_cnt_time = 1;
/* to store the address of callback function */
void (*callback_func)();

/**
 * @brief Initialize timer.
 * @param tickback the address of call back function.
 */
void init_timer(void (*tickback)(unsigned int)) {
    unsigned short cycles_per_interrupt =
        (TIMER_RATE / (MS_PER_S / MS_PER_INTERRUPT));
    /* set to zero */
    outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
    /* set the frequency */
    outb(TIMER_PERIOD_IO_PORT, LSB(cycles_per_interrupt));
    outb(TIMER_PERIOD_IO_PORT, MSB(cycles_per_interrupt));
    /* initiallize callback function */
    numTicks = 0;
    callback_func = tickback;
}

/**
 * @brief Increment numTicks per 10 ms and set callback function.
 */
void ticktock() {
    numTicks++;
    callback_func(numTicks);
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
}

/**
 * @brief Callback function that is called by the handler.
 * @param numTicks the number of 10 ms that is triggered.
 */
void cnt_seconds(unsigned int numTicks) {
    if (numTicks % 100 == 0 && if_cnt_time)
        seconds++;
}

/**
 * @brief get current time.
 * @return current time.
 */
int get_seconds() {
    return seconds;
}
