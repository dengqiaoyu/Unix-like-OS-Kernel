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

/* debug include */
#include <simics.h>             /* lprintf() */

/* user defined include */
#include "keyboard_driver.h"
#include "scheduler.h"

static uint8_t buf[KB_BUF_LEN] = {0};
/* the position where the last character can be read */
static int buf_ending = 0;
/* the position where the last character has been read */
static int buf_cursor = 0;

// static int temp_ctx_cnter = 0;

/* functions definition */
/**
 * @brief Read from keyboard port and put it into a buffer for further handling.
 */
void add_to_kb_buf(void) {
    // temp_ctx_cnter++;
    // lprintf("temp_ctx_cnter: %d\n", temp_ctx_cnter);
    // if (temp_ctx_cnter % 2 == 0) {
    //     outb(INT_ACK_CURRENT, INT_CTL_PORT);
    //     return;
    // }
    // uint8_t temp_code = inb(KEYBOARD_PORT);
    if (buf_ending + 1 == buf_cursor)
        return;
    buf[buf_ending] = inb(KEYBOARD_PORT);
    buf_ending = (buf_ending + 1) % KB_BUF_LEN;
    // char ch = readchar();
    // lprintf("keyboard pressed, %d\n", temp_code);
    // if (ch == 'a') {
    //     outb(INT_ACK_CURRENT, INT_CTL_PORT);
    //     sche_yield();
    // }
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
}

/**
 * @brief Read from the buffer. If it is a character that can be printed, return
 *        it.
 * @return  the integer value of character.
 */
int readchar(void) {
    uint8_t keypress = 0;
    /* TODO Cannot decide whether to use disable_interrupts to avoid the race
     *      condition if one is only writing and the other one is only reading.
     *      I think there is no need for disable all the interrupt.
     */
    // disable_interrupts();
    if (buf_cursor == buf_ending) {
        return -1;
    }
    // enable_interrupts();

    keypress = buf[buf_cursor];
    buf_cursor = (buf_cursor + 1) % KB_BUF_LEN;
    kh_type augchar = process_scancode(keypress);
    if (KH_HASDATA(augchar) && !KH_ISMAKE(augchar))
        return KH_GETCHAR(augchar);

    return -1;
}
