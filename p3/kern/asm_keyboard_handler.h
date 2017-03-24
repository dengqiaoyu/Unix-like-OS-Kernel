/**
 * @file keyboard_handler.h
 * @brief A header file contains asm function declarations of
 *        keyboard_handler_asm.S
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef _H_KEYBOARD_HANDLER_
#define _H_KEYBOARD_HANDLER_

/**
 * @brief keyboard handler that get called by the IDT entry.
 */
void keyboard_handler(void);

#endif /* _H_KEYBOARD_HANDLER_ */