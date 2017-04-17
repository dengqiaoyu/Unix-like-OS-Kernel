#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */
#include <console.h>

#include "syscalls/syscalls.h"
#include "drivers/keyboard_driver.h"
#include "vm.h"
#include "utils/kern_mutex.h"
#include "utils/kern_cond.h"
#include "utils/kern_sem.h"

extern keyboard_buffer_t kb_buf;

int kern_readline(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int len = (int)(*esi);
    char *buf = (char *)(*(esi + 1));
    if (len > 4096) return -1;
    /* check buf */
    int ret = validate_user_mem((uint32_t)buf, len, MAP_USER | MAP_WRITE);
    if (ret < 0) return -1;

    kern_sem_wait(&kb_buf.readline_sem);
    kern_mutex_lock(&kb_buf.mutex);
    disable_interrupts();
    if (kb_buf.newline_cnt == 0) {
        int kb_buf_ending = kb_buf.buf_ending;
        enable_interrupts();
        kb_buf.is_waiting = 1;
        for (int i = kb_buf.buf_start;
                i < kb_buf_ending;
                i = (i + 1) % KB_BUF_LEN) {
            putbyte(kb_buf.buf[i]);
        }
        kern_cond_wait(&kb_buf.cond, &kb_buf.mutex);
    } else enable_interrupts();

    int if_print = 1;
    if (kb_buf.is_waiting) {
        if_print = 0;
        kb_buf.is_waiting = 0;
    }
    int kb_buf_ending = kb_buf.buf_ending;
    kern_mutex_unlock(&kb_buf.mutex);
    int actual_len = 0;
    while (kb_buf.buf_start < kb_buf_ending) {
        int new_kb_buf_start = (kb_buf.buf_start + 1) % KB_BUF_LEN;
        char ch = kb_buf.buf[kb_buf.buf_start];
        kb_buf.buf_start = new_kb_buf_start;
        buf[actual_len++] = ch;
        if (if_print) putbyte(ch);
        if (ch == '\n') {
            if (actual_len < len) buf[actual_len] = '\0';
            if (actual_len == len) buf[actual_len - 1] = '\0';
            kern_mutex_lock(&kb_buf.mutex);
            kb_buf.newline_cnt--;
            kern_mutex_unlock(&kb_buf.mutex);
            break;
        }
        if (actual_len == len - 1) {
            buf[actual_len] = '\0';
            break;
        }
    }
    kern_sem_signal(&kb_buf.readline_sem);

    return 0;
}

int kern_print(void) {
    // TODO DEBUG
    return 0;

    uint32_t *esi = (uint32_t *)asm_get_esi();
    int len = (int)(*esi);
    char *buf = (char *)(*(esi + 1));

    // TODO macro megabyte
    if (len > 1024 * 1024) return -1;
    int ret = validate_user_mem((uint32_t)buf, len, MAP_USER);
    if (ret < 0) return -1;

    putbytes(buf, len);
    return 0;
}

int kern_set_term_color(void) {
    int color = (int)asm_get_esi();

    if (color & ~COLOR_MASK) return -1;
    return set_term_color(color);
}

int kern_set_cursor_pos(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int row = (int)(*esi);
    int col = (int)(*(esi + 1));

    return set_cursor(row, col);
}

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
