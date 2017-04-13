/**
 * @file keyboard_driver.h
 * @brief A header file contains function declarations of keyboard_driver.h
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_KEYBOARD_DRIVER
#define H_KEYBOARD_DRIVER

#include <mutex.h>
#include "kern_cond.h"
#include "kern_sem.h"

#define KB_BUF_LEN 256

typedef struct keyboard_buffer {
    char buf[KB_BUF_LEN];
    int buf_start;
    int buf_ending;
    int newline_cnt;
    int is_waiting;
    mutex_t mutex;
    kern_cond_t cond;
    kern_sem_t readline_sem;
} keyboard_buffer_t;

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