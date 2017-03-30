/** @file asm_interrupts.h
 *  @brief A header file contains asm function declarations of
 *  @author Qioayu Deng
 *  @andrew_id qdeng
 *  @bug No known bugs.
 */

#ifndef _ASM_INTERRUPTS_H_
#define _ASM_INTERRUPTS_H_

/**
 * @brief keyboard handler that get called by the IDT entry.
 */
void keyboard_handler(void);

/**
 * @brief timer handler that get called by the IDT entry.
 */
void timer_handler(void);

#endif
