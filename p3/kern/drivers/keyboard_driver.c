/**
 *  @file keyboard_driver.c
 *  @brief A keyboard driver that can read char into buffer and then do further
 *         operation.
 *  @author Qiaoyu Deng
 *  @andrew_id qdeng
 *  @bug No known bugs.
 */

#include <keyhelp.h>

/* x86 includes */
#include <asm.h>                /* disable_interrupts(), enable_interrupts() */
#include <interrupt_defines.h>  /* INT_ACK_CURRENT, INT_CTL_PORT */
#include <console.h>
/* debug include */
#include <simics.h>             /* lprintf() */

/* user defined include */
#include "drivers/keyboard_driver.h"
#include "scheduler.h"



/* internal function */
int _process_keypress(uint8_t keypress);

// static int temp_ctx_cnter = 0;

/* functions definition */

keyboard_buffer_t kb_buf;

int keyboard_init() {
    kb_buf.buf_start = 0;
    kb_buf.buf_ending = 0;
    kb_buf.newline_cnt = 0;
    kb_buf.is_waiting = 0;
    kern_mutex_init(&kb_buf.mutex);
    kern_cond_init(&kb_buf.cond);
    kern_sem_init(&kb_buf.readline_sem, 1);
    return 0;
}

/**
 * @brief Read from keyboard port and put it into a buffer for further handling.
 */
void add_to_kb_buf(void) {
    // lprintf("entering keyboard");
    uint8_t keypress = inb(KEYBOARD_PORT);
    char ch = _process_keypress(keypress);
    if (ch == -1) {
        outb(INT_ACK_CURRENT, INT_CTL_PORT);
        return;
    }
    int new_buf_ending = 0;
    int if_newline = 0;
    switch (ch) {
    case '\b':
        if (kb_buf.buf_start == kb_buf.buf_ending) {
            outb(INT_ACK_CURRENT, INT_CTL_PORT);
            return;
        }
        new_buf_ending = (kb_buf.buf_ending - 1) % KB_BUF_LEN;
        kb_buf.buf_ending = new_buf_ending;
        break;
    case '\n':
        if_newline = 1;
        new_buf_ending = (kb_buf.buf_ending + 1) % KB_BUF_LEN;
        if (new_buf_ending == kb_buf.buf_start) {
            new_buf_ending = (kb_buf.buf_ending - 1) % KB_BUF_LEN;
            kb_buf.buf[new_buf_ending] = ch;
        } else {
            kb_buf.buf[kb_buf.buf_ending] = ch;
            kb_buf.buf_ending = new_buf_ending;
        }
        // lprintf("%c get into buffer", ch);
        break;
    default:
        new_buf_ending = (kb_buf.buf_ending + 1) % KB_BUF_LEN;
        if (new_buf_ending == kb_buf.buf_start) break;
        kb_buf.buf[kb_buf.buf_ending] = ch;
        kb_buf.buf_ending = new_buf_ending;
        // lprintf("%c get into buffer", ch);
        break;
    }
    /* DEBUG use */
    // int i = 0;
    // lprintf("#######Buffer Content#######");
    // for (i = kb_buf.buf_start; i < kb_buf.buf_ending; i++)
    //     lprintf("%c", kb_buf.buf[i]);
    // lprintf("#######Buffer Content#######");
    /* DEBUG use */
    outb(INT_ACK_CURRENT, INT_CTL_PORT);

    kern_mutex_lock(&kb_buf.mutex);
    if (kb_buf.is_waiting) {
        putbyte(ch);
    }
    if (if_newline) {
        kb_buf.newline_cnt++;
        kern_cond_signal(&kb_buf.cond);
    }
    kern_mutex_unlock(&kb_buf.mutex);
}

int _process_keypress(uint8_t keypress) {
    kh_type augchar = process_scancode(keypress);
    if (KH_HASDATA(augchar) && !KH_ISMAKE(augchar))
        return KH_GETCHAR(augchar);
    return -1;
}
