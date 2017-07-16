/**
 * @file keyboard_handler.h
 * @brief A header file contains asm function declarations of
 *        keyboard_handler_asm.S
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_KEYBOARD_HANDLER
#define H_KEYBOARD_HANDLER

/**
 * @brief keyboard handler that get called by the IDT entry.
 */
void keyboard_handler(void);

#endif