/**
 * @file drivers/keyboard_driver.h
 * @brief A header file contains function declarations of drivers/keyboard_driver.h
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_KEYBOARD_DRIVER
#define H_KEYBOARD_DRIVER

#include "utils/mutex.h"
#include "utils/kern_cond.h"
#include "utils/kern_sem.h"

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

int kb_buf_init();

/**
 * @brief Read from keyboard port and put it into a buffer for further handling.
 */
void add_to_kb_buf();

#endif