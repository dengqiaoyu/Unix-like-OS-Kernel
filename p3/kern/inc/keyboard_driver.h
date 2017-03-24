/**
 * @file keyboard_driver.h
 * @brief A header file contains function declarations of keyboard_driver.h
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_KEYBOARD_DRIVER
#define H_KEYBOARD_DRIVER

#define KB_BUF_LEN 4096

/* function declarations */
/**
 * @brief Read from keyboard port and put it into a buffer for further handling.
 */
void add_to_kb_buf();

/**
 * @brief Read from the buffer. If it is a character that can be printed, return
 *        it.
 * @return  the integer value of character.
 */
int readchar();

#endif