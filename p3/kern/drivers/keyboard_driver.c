/**
 *  @file   keyboard_driver.c
 *  @brief  A keyboard driver that can read char into buffer and then do further
 *          operation.
 *  @author Qiaoyu Deng (qdeng)
 *  @bug    No known bugs.
 */
#include <keyhelp.h>

/* libc includes */
#include <asm.h>                /* disable_interrupts(), enable_interrupts() */
#include <interrupt_defines.h>  /* INT_ACK_CURRENT, INT_CTL_PORT */
#include <console.h>

/* DEBUG */
#include <simics.h>             /* lprintf() */

/* user defined include */
#include "drivers/keyboard_driver.h"
#include "scheduler.h"

/* internal function */
int _process_keypress(uint8_t keypress);

/* functions definition */
keyboard_buffer_t kb_buf;

/**
 * Initialize ketboard input buffer
 * @return 0 as success, -1 as failure
 */
int keyboard_init() {
    kb_buf.buf_start = 0;
    kb_buf.buf_ending = 0;
    kb_buf.newline_cnt = 0;
    kb_buf.is_waiting = 0;
    if (kern_mutex_init(&kb_buf.mutex) != 0) return -1;
    if (kern_cond_init(&kb_buf.cond) != 0) return -1;
    if (kern_sem_init(&kb_buf.readline_sem, 1) != 0) return -1;
    return 0;
}

/**
 * @brief Read from keyboard port and put it into a buffer for further handling.
 */
void add_to_kb_buf(void) {
    uint8_t keypress = inb(KEYBOARD_PORT);
    /* convert keyboard press into character */
    char ch = _process_keypress(keypress);
    if (ch == -1) {
        /* not a character */
        outb(INT_ACK_CURRENT, INT_CTL_PORT);
        return;
    }
    int new_buf_ending = 0;
    /* whether new line character is encountered */
    int if_newline = 0;
    switch (ch) {
    case '\b':
        if (kb_buf.buf_start == kb_buf.buf_ending) {
            /* buffer is empty */
            outb(INT_ACK_CURRENT, INT_CTL_PORT);
            return;
        }
        /* delete one character */
        new_buf_ending = (kb_buf.buf_ending - 1) % KB_BUF_LEN;
        kb_buf.buf_ending = new_buf_ending;
        break;
    case '\n':
        /* encounter a new line character */
        if_newline = 1;
        /* calculate next ending by mod */
        new_buf_ending = (kb_buf.buf_ending + 1) % KB_BUF_LEN;
        if (new_buf_ending == kb_buf.buf_start) {
            /* if buffer is full, replace the last one with new line */
            new_buf_ending = (kb_buf.buf_ending - 1) % KB_BUF_LEN;
            kb_buf.buf[new_buf_ending] = ch;
        } else {
            /* put it into buffer */
            kb_buf.buf[kb_buf.buf_ending] = ch;
            kb_buf.buf_ending = new_buf_ending;
        }
        break;
    default:
        /* normal character */
        new_buf_ending = (kb_buf.buf_ending + 1) % KB_BUF_LEN;
        /* if buffer is full */
        if (new_buf_ending == kb_buf.buf_start) return;
        kb_buf.buf[kb_buf.buf_ending] = ch;
        kb_buf.buf_ending = new_buf_ending;
        break;
    }
    /* report port here, then we can get more input */
    outb(INT_ACK_CURRENT, INT_CTL_PORT);

    /* allow this block of code be interrupted by keyboard, when new input in */
    kern_mutex_lock(&kb_buf.mutex);
    if (kb_buf.is_waiting) {
        /* if some thread is waiting on keyboard input */
        putbyte(ch);
    }
    if (if_newline) {
        /* encounter a new line character */
        kb_buf.newline_cnt++;
        /* wake up the waiting thread */
        kern_cond_signal(&kb_buf.cond);
    }
    kern_mutex_unlock(&kb_buf.mutex);
}

/**
 * @brief           process keyboard code to char
 * @param  keypress keyboard code from hardware IO
 * @return          char if it is a displayable character, -1 otherwise
 */
int _process_keypress(uint8_t keypress) {
    kh_type augchar = process_scancode(keypress);
    if (KH_HASDATA(augchar) && !KH_ISMAKE(augchar))
        return KH_GETCHAR(augchar);
    return -1;
}
