/**
 * @file timer_driver.h
 * @brief A header file contains function declarations of timer_driver.c
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_TIMER_DRIVER
#define H_TIMER_DRIVER

#define MS_PER_INTERRUPT 10
#define MS_PER_S 1000

#define LSB(addr) (addr & 0x00ff)
#define MSB(addr) ((addr & 0xff00) >> 8)

/**
 * @brief Initialize timer.
 * @param tickback the address of call back function.
 */
void timer_init(void (*tickback)(unsigned int));

/**
 * @brief Callback function that is called by the handler.
 * @param numTicks the number of TIMER_RATE that is triggered.
 */
void timer_callback(unsigned int numTicks);

/**
 * @brief get current time.
 * @return current time.
 */
int get_seconds();

#endif
