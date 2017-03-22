/**
 * @file timer_handler.h
 * @brief A header file contains asm function declarations of
 *        timer_handler_asm.S
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef _H_TIMER_HANDLER_
#define _H_TIMER_HANDLER_

/**
 * @brief timer handler that get called by the IDT entry.
 */
void timer_handler(void);

#endif /* _H_TIMER_HANDLER_ */