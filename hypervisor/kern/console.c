/** @file   console.c
 *  @brief  Implementation of the console driver API.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug    No known bugs.
 */

#include <ctype.h>
#include <string.h>

#include <x86/asm.h>
#include <x86/video_defines.h>
#include <malloc.h>
#include <assert.h>
#include <console.h>

static int cursor_row = 0;
static int cursor_col = 0;
static int console_color = FGND_WHITE | BGND_BLACK;
static int cursor_hidden = 0;

/* for backing up main console state */
static void *console_mem_back = NULL;
static int cursor_row_back = 0;
static int cursor_col_back = 0;

/** @brief Scrolls the console down one line
 *
 *  Takes everything except the first row in console mapped memory and shifts
 *  down by one row. Then, the last row is filled in with blanks.
 *
 *  @return void
 **/
static void scroll_down() {
    char *addr = (char *)CONSOLE_MEM_BASE;

    int row;
    for (row = 0; row < CONSOLE_HEIGHT - 1; row++) {
        memcpy(addr, addr + 2 * CONSOLE_WIDTH, 2 * CONSOLE_WIDTH);
        addr += 2 * CONSOLE_WIDTH;
    }

    int col;
    for (col = 0; col < CONSOLE_WIDTH; col++) {
        *(addr++) = ' ';
        *(addr++) = (char)console_color;
    }
}

/** @brief Moves the cursor to a new line
 *
 *  Calls scroll_down() if the cursor is at the bottom of the console screen.
 *  Otherwise, the cursor row is incremented. Resets the cursor column to 0.
 *
 *  @return void
 **/
static void new_line() {
    if (cursor_row == CONSOLE_HEIGHT - 1) scroll_down();
    else cursor_row++;
    cursor_col = 0;
}

/** @brief Sends cursor position to the CRTC
 *
 *  Sends cursor position data to the CRTC so that the position is updated on
 *  the console screen. If the cursor should be hidden, the position will be
 *  moved off screen. Otherwise, the new cursor position's color is set to the
 *  current console color so that the cursor reflects the appearance of future
 *  printed characters.
 *
 *  @return void
 **/
static void update_cursor() {
    int cursor_pos = cursor_row * CONSOLE_WIDTH + cursor_col;
    if (cursor_hidden) {
        cursor_pos += CONSOLE_HEIGHT * CONSOLE_WIDTH;
    } else {
        char *addr = (char *)CONSOLE_MEM_BASE;
        addr += 2 * cursor_pos;
        *(addr + 1) = (char)console_color;
    }

    outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
    outb(CRTC_DATA_REG, cursor_pos & 0xff);
    outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
    outb(CRTC_DATA_REG, cursor_pos >> 8);
}

int putbyte( char ch ) {
    if (ch == '\r') cursor_col = 0;
    else if (ch == '\n') new_line();
    else if (ch == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            draw_char(cursor_row, cursor_col, ' ', console_color);
        }
    } else {
        draw_char(cursor_row, cursor_col, ch, console_color);
        if (cursor_col == CONSOLE_WIDTH - 1) new_line();
        else cursor_col++;
    }

    update_cursor();
    return (int)ch;
}

void putbytes( const char *s, int len ) {
    if (s == 0 || len <= 0) return;
    int i;
    for (i = 0; i < len; i++) putbyte(s[i]);
}

int set_term_color( int color ) {
    if (color & ~0xff) return -1;
    console_color = color;
    update_cursor();
    return 0;
}

void get_term_color( int *color ) {
    if (!color) return;
    *color = console_color;
}

int set_cursor( int row, int col ) {
    if (row < 0 || row >= CONSOLE_HEIGHT) return -1;
    if (col < 0 || col >= CONSOLE_WIDTH) return -1;
    cursor_row = row;
    cursor_col = col;
    update_cursor();
    return 0;
}

void get_cursor( int *row, int *col ) {
    if (!row || !col) return;
    *row = cursor_row;
    *col = cursor_col;
}

void hide_cursor() {
    cursor_hidden = 1;
    update_cursor();
}

void show_cursor() {
    cursor_hidden = 0;
    update_cursor();
}

void clear_console() {
    char *base = (char *)CONSOLE_MEM_BASE;
    char *addr = base;

    // clear first line
    int col;
    for (col = 0; col < CONSOLE_WIDTH; col++) {
        *(addr++) = ' ';
        *(addr++) = (char)console_color;
    }

    // now we can just copy the first line repeatedly
    int row;
    for (row = 0; row < CONSOLE_HEIGHT - 1; row++) {
        memcpy(addr, base, 2 * CONSOLE_WIDTH);
        addr += 2 * CONSOLE_WIDTH;
    }

    set_cursor(0, 0);
}

void draw_char( int row, int col, int ch, int color ) {
    if (row < 0 || row >= CONSOLE_HEIGHT) return;
    if (col < 0 || col >= CONSOLE_WIDTH) return;
    if (!isprint(ch)) return;
    if (color & ~0xff) return;

    int addr = CONSOLE_MEM_BASE;
    addr += 2 * (row * CONSOLE_WIDTH + col);
    *(char *)addr = (char)ch;
    *(char *)(addr + 1) = (char)color;
}

char get_char( int row, int col ) {
    if (row < 0 || row >= CONSOLE_HEIGHT) return 0;
    if (col < 0 || col >= CONSOLE_WIDTH) return 0;
    int addr = CONSOLE_MEM_BASE;
    addr += 2 * (row * CONSOLE_WIDTH + col);
    return *(char *)addr;
}

int backup_main_console() {
    assert(console_mem_back == NULL);
    console_mem_back = malloc(CONSOLE_MEM_SIZE);
    if (console_mem_back == NULL) return -1;

    memcpy(console_mem_back, (void *)CONSOLE_MEM_BASE, CONSOLE_MEM_SIZE);
    cursor_row_back = cursor_row;
    cursor_col_back = cursor_col;
    return 0;
}

void restore_main_console() {
    assert(console_mem_back != NULL);
    memcpy((void *)CONSOLE_MEM_BASE, console_mem_back, CONSOLE_MEM_SIZE);
    cursor_row = cursor_row_back;
    cursor_col = cursor_col_back;

    free(console_mem_back);
    console_mem_back = NULL;
    update_cursor();
}

console_state_t *backup_console() {
    // TODO for virtual consoles
    return NULL;
}

void restore_console(console_state_t *backup) {
    // TODO for virtual consoles
    return;
}
