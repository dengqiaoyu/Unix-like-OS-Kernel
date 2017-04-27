/**
 * @file drivers/keyboard_driver.h
 * @brief A header file contains function declarations of drivers/keyboard_driver.h
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_KEYBOARD_DRIVER
#define H_KEYBOARD_DRIVER

#include "utils/kern_mutex.h"
#include "utils/kern_cond.h"
#include "utils/kern_sem.h"

#define KB_BUF_LEN 256

typedef struct keyboard_buffer {
    char buf[KB_BUF_LEN];
    int buf_start;
    int buf_ending;
    kern_mutex_t wait_mutex;
    kern_sem_t console_io_sem;
    kern_cond_t new_input_cond;
} keyboard_buffer_t;

/* function declarations */

int keyboard_init();

/**
 * @brief Read from keyboard port and put it into a buffer for further handling.
 */
void add_to_kb_buf();

#endif