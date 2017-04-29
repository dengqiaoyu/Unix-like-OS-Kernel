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
#define EXECNAME_MAX 64

/* keyboard input buffer */
extern keyboard_buffer_t kb_buf;
/* mutex make print thread safe */
kern_mutex_t print_mutex;

char _getchar(void);

/**
 * kernel implementation of syscall getchar.
 * @return  char value of input
 */
char kern_getchar(void) {
    char ch = 0;
    kern_mutex_lock(&kb_buf.wait_mutex);
    ch = _getchar();
    kern_mutex_unlock(&kb_buf.wait_mutex);
    return ch;
}

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

    char ch = 0;
    int actual_len = 0;
    int if_end = 0;
    /* check buf validation */
    int ret = validate_user_mem((uint32_t)buf, len, MAP_USER | MAP_WRITE);
    if (ret < 0) return -1;

    kern_mutex_lock(&kb_buf.wait_mutex);

    while (!if_end) {
        ch = _getchar();
        switch (ch) {
        case '\b':
            if (actual_len == 0) break;
            actual_len--;
            break;
        case '\n':
            if_end = 1;
        /* fall through */
        default:
            if (actual_len < len) buf[actual_len++] = ch;
            break;
        }
        putbyte(ch);
    }
    if (actual_len < len) buf[actual_len] = '\0';
    if (actual_len == len) buf[actual_len - 1] = '\0';
    kern_mutex_unlock(&kb_buf.wait_mutex);

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

int kern_readfile(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    char *filename = (char *)(*esi);
    char *buf = (char *)(*(esi + 1));
    int count = (int)(*(esi + 2));
    int offset = (int)(*(esi + 3));

    if (count < 0) return -1;

    int ret;
    // check whether filename is valid
    ret = validate_user_string((uint32_t)filename, EXECNAME_MAX);
    if (ret <= 0) return -1;

    if (count > 0) {
        // no need to validate user memory if we aren't copying anything
        ret = validate_user_mem((uint32_t)buf, count, MAP_USER | MAP_WRITE);
        if (ret < 0) return -1;
    }

    ret = getbytes(filename, offset, count, buf);
    return ret;
}

/* helper function */
char _getchar(void) {
    char ch = 0;
    kern_sem_wait(&kb_buf.console_io_sem);
    int new_kb_buf_start = (kb_buf.buf_start + 1) % KB_BUF_LEN;
    ch = kb_buf.buf[kb_buf.buf_start];
    kb_buf.buf_start = new_kb_buf_start;
    return ch;
}
