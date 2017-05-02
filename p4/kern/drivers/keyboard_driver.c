/**
 *  @file   keyboard_driver.c
 *  @brief  A keyboard driver that can read char into buffer and then do further
 *          operation.
 *  @author Qiaoyu Deng (qdeng)
 *  @bug    No known bugs.
 */
#include <stdlib.h>
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

guest_info_t *guest_info_for_kb = NULL;

/**
 * Initialize ketboard input buffer
 * @return 0 as success, -1 as failure
 */
int keyboard_init() {
    kb_buf.buf_start = 0;
    kb_buf.buf_ending = 0;
    if (kern_mutex_init(&kb_buf.wait_mutex) != 0) return -1;
    if (kern_cond_init(&kb_buf.new_input_cond) != 0) return -1;
    if (kern_sem_init(&kb_buf.console_io_sem, 0) != 0) return -1;
    return 0;
}

/**
 * @brief Read from keyboard port and put it into a buffer for further handling.
 */
void add_to_kb_buf(void) {
    uint8_t keypress = inb(KEYBOARD_PORT);
    if (guest_info_for_kb != NULL) {
        int new_buf_end = (guest_info_for_kb->buf_end + 1) % KC_BUF_LEN;
        if (new_buf_end == guest_info_for_kb->buf_start) {
            outb(INT_ACK_CURRENT, INT_CTL_PORT);
            return;
        }
        guest_info_for_kb->keycode_buf[guest_info_for_kb->buf_end] = keypress;
        /* set iret go to keyboard handler */
        guest_info_for_kb->buf_end = new_buf_end;
        outb(INT_ACK_CURRENT, INT_CTL_PORT);
        return;
    }
    /* convert keyboard press into character */
    char ch = _process_keypress(keypress);
    if (ch == -1) {
        /* not a character */
        outb(INT_ACK_CURRENT, INT_CTL_PORT);
        return;
    }
    int new_buf_ending = 0;
    new_buf_ending = (kb_buf.buf_ending + 1) % KB_BUF_LEN;
    /* if buffer is full */
    if (new_buf_ending == kb_buf.buf_start) {
        outb(INT_ACK_CURRENT, INT_CTL_PORT);
        return;
    }
    kb_buf.buf[kb_buf.buf_ending] = ch;
    kb_buf.buf_ending = new_buf_ending;
    /* report port here, then we can get more input */
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
    kern_sem_signal(&kb_buf.console_io_sem);
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
