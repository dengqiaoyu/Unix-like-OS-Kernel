/**
 * @file console.c
 * @brief This file contains the basic system calls for operate console.
 * @author Newton Xie (ncx)
 * @author Qiaoyu Deng (qdeng)
 * @bug No bugs
 */
/* libc includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */
#include <console.h>

#include "vm.h"
#include "syscalls/syscalls.h"
#include "drivers/keyboard_driver.h"
#include "utils/kern_mutex.h"
#include "utils/kern_cond.h"
#include "utils/kern_sem.h"

#define MEGABYTES (1024 * 1024)

/* keyboard input buffer */
extern keyboard_buffer_t kb_buf;
/* mutex make print thread safe */
kern_mutex_t print_mutex;


/**
 * Reads the next line from the console and copies it into the buffer pointed
 * to by buf.
 *
 * If there is no line of input currently available, the calling thread is
 * descheduled until one is. If some other thread is descheduled on a readline()
 * or a getchar(), then the calling thread must block and wait its turn to
 * access the input stream. The length of the buffer is indicated by len. If
 * the line is smaller than the buffer, then the complete line including the
 * newline character is copied into the buffer. If the length of the line
 * exceeds the length of the buffer, only len characters should be copied into
 * buf.
 * @param  size the size of provided buffer
 * @param  buf  buffer address
 * @return      number of characters that are put into buffer, -1 if the buffer
 *              is invalid
 */
int kern_readline(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int len = (int)(*esi);
    char *buf = (char *)(*(esi + 1));
    if (len > 4096 || len <= 0) return -1;
    /* check buf validation */
    int ret = validate_user_mem((uint32_t)buf, len, MAP_USER | MAP_WRITE);
    if (ret < 0) return -1;

    /* each time there should be only one thread reading input */
    kern_sem_wait(&kb_buf.readline_sem);
    kern_mutex_lock(&kb_buf.mutex);
    /**
     * There should be a disable_interrupts because we need to ensure there is
     * no new line entering to buffer after we check them, if we check new line
     * count is 0, we need to wait on cond_var to be wake up again when a new
     * line character is available.
     */
    disable_interrupts();
    if (kb_buf.newline_cnt == 0) {
        int kb_buf_ending = kb_buf.buf_ending;
        enable_interrupts();
        kb_buf.is_waiting = 1;
        /* if there is input available, just print it to screen */
        for (int i = kb_buf.buf_start;
                i < kb_buf_ending;
                i = (i + 1) % KB_BUF_LEN) {
            putbyte(kb_buf.buf[i]);
        }
        /* wait to be waked up when new line character comes */
        kern_cond_wait(&kb_buf.cond, &kb_buf.mutex);
    } else enable_interrupts();

    /**
     * If we have already for new line character to come, which means we have
     * wait on readline for a while, which means keyboard driver print character
     * for us, we do not need print it again when we copy it to buffer. And also
     * set if_waiting to 0, to let keyboard driver know and does not print new
     * character.
     */
    int if_print = 1;
    if (kb_buf.is_waiting) {
        if_print = 0;
        kb_buf.is_waiting = 0;
    }
    /**
     * Up until now, before buffer ends, we must encounter a newline character
     * or fill the buffer to full, it will not affect us if we get new character
     * when we copy fro old one.
     */
    int kb_buf_ending = kb_buf.buf_ending;
    kern_mutex_unlock(&kb_buf.mutex);
    int actual_len = 0;
    while (kb_buf.buf_start < kb_buf_ending) {
        int new_kb_buf_start = (kb_buf.buf_start + 1) % KB_BUF_LEN;
        char ch = kb_buf.buf[kb_buf.buf_start];
        /* move the cursor */
        kb_buf.buf_start = new_kb_buf_start;
        buf[actual_len++] = ch;
        /* if already print when waiting on input */
        if (if_print) putbyte(ch);
        if (ch == '\n') {
            /* make buffer null terminated */
            if (actual_len < len) buf[actual_len] = '\0';
            if (actual_len == len) buf[actual_len - 1] = '\0';
            kern_mutex_lock(&kb_buf.mutex);
            kb_buf.newline_cnt--;
            kern_mutex_unlock(&kb_buf.mutex);
            break;
        }
        /* make buffer null terminated */
        if (actual_len == len - 1) {
            buf[actual_len] = '\0';
            break;
        }
    }
    /* let next thread entering readline */
    kern_sem_signal(&kb_buf.readline_sem);
    return actual_len;
}

/**
 * Prints len bytes of memory, starting at buf, to the console and The calling
 * thread should not continue until all characters have been printed to the
 * console.
 * @param  size the length of buffer
 * @param  buf  buffer that contains
 * @return      0 as success, -1 as failure
 */
int kern_print(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int len = (int)(*esi);
    char *buf = (char *)(*(esi + 1));

    // TODO macro megabyte
    if (len > MEGABYTES) return -1;
    int ret = validate_user_mem((uint32_t)buf, len, MAP_USER);
    if (ret < 0) return -1;

    /* make it thread safe */
    kern_mutex_lock(&print_mutex);
    putbytes(buf, len);
    kern_mutex_unlock(&print_mutex);
    return 0;
}

/**
 * Sets the terminal print color for any future output to the console. If color
 * does not specify a valid color, an integer error code less than zero should
 * be returned.
 * @param  color color code
 * @return       0 as success, -1 as failure
 */
int kern_set_term_color(void) {
    int color = (int)asm_get_esi();

    if (color & ~COLOR_MASK) return -1;
    return set_term_color(color);
}

/**
 * Sets the cursor to the location (row, col).
 * @param  row the row of cursor after this call
 * @param  col the col of cursor after this call
 * @return     0 as success, -1 as failure
 */
int kern_set_cursor_pos(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int row = (int)(*esi);
    int col = (int)(*(esi + 1));

    return set_cursor(row, col);
}

/**
 * Writes the current location of the cursor to the integers addressed by the
 * two arguments.
 * @param  row the address of row that the row number will written into
 * @param  col the address of col that the col number will written into
 * @return     0 as success, -1 as failure
 */
int kern_get_cursor_pos(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int *row = (int *)(*esi);
    int *col = (int *)(*(esi + 1));

    int ret;
    ret = validate_user_mem((uint32_t)row, sizeof(int), MAP_USER | MAP_WRITE);
    if (ret < 0) return -1;
    ret = validate_user_mem((uint32_t)col, sizeof(int), MAP_USER | MAP_WRITE);
    if (ret < 0) return -1;

    get_cursor(row, col);
    return 0;
}
