/**
 *  @file console_driver.c
 *  @brief A console driver for manipulating screen.
 *  @author Qiaoyu Deng
 *  @andrew_id qdeng
 *  @bug No known bugs.
 */

#include <p1kern.h>
/* libc include */
#include <stdio.h>

/* x86 specific include */
#include <x86/asm.h>    /* outb() */

/* debug include */
#include <simics.h>     /* lprintf() */

/* user define includes */
#include "inc/console_driver.h"

static int logic_cursor = 0; /* store the position of logical cursor */
static int term_color = BGND_BLACK | FGND_WHITE; /* default color scheme */
static int is_hide = 0; /* control whether to hide the cursor */
static int is_allowed_scroll = 1; /* control whether allow scroll the screen */

/**
 * @brief Prints the character array s, starting at the current location of the
 *        cursor. If there are more characters than there are spaces remaining
 *        on the current line, the characters should fill up the current line
 *        and then continue on the next line. If the characters exceed available
 *        space on the entire console, the screen should scroll up one line,
 *        and then printing should continue on the new line. If '\n', '\r',
 *        and '\b' are encountered within the string, they should be handled as
 *        per putbyte. If len is not a positive integer or s is null,
 *        the function has no effect.
 * @param  ch the character to be printed.
 * @return    the input character.
 */
int putbyte(char ch) {
    int cur_row = 0;
    int cur_col = 0;
    get_cursor(&cur_row, &cur_col);

    switch (ch) {
    case '\n':
        /* if cursor is at last line of the screen. */
        if (cur_row == CONSOLE_HEIGHT - 1) {
            if (is_allowed_scroll) {
                scroll_lines(1);
            }
        } else {
            cur_row++;
        }
        cur_col = 0;
        break;
    case '\r':
        cur_col = 0;
        break;
    case '\b':
        /* if cursor is not at the beginning of the screen. */
        if (logic_cursor > 0)
            logic_cursor--;
        if (cur_col == 0) {
            find_line_ending();
        } else {
            draw_char(cur_row, cur_col - 1, 0, term_color);
        }
        get_cursor(&cur_row, &cur_col);
        break;
    default:
        /* print the character at the position that cursor in. */
        draw_char(cur_row, cur_col, ch, term_color);
        if (logic_cursor == SCREEN_LEN - 1) {
            if (is_allowed_scroll) {
                scroll_lines(1);
                logic_cursor = SCREEN_LEN - CONSOLE_WIDTH;
            } else {
                logic_cursor = 0;
            }
        } else {
            logic_cursor++;
        }
        get_cursor(&cur_row, &cur_col);
        break;
    }

    set_cursor(cur_row, cur_col);
    return ch;
}

/**
 * @brief Prints character ch at the current location of the cursor. If the
 *        character is a newline ('\n'), the cursor will be moved to the
 *        first column of the next line (scrolling if necessary). If the
 *        character is a carriage return ('\r'), the cursor will be
 *        immediately reset to the beginning of the current line, causing any
 *        future output to overwrite any existing output on the line.
 *        If backspace ('\b') is encountered, the previous character will be
 *        erased.
 * @param s   the character array to be printed.
 * @param len the number of characters to print from the array.
 */
void putbytes(const char *s, int len) {
    int i;
    int cur_row = 0;
    int cur_col = 0;
    for (i = 0; i < len; i++) {
        get_cursor(&cur_row, &cur_col);
        switch (s[i]) {
        case '\n':
            if (cur_row == CONSOLE_HEIGHT - 1) {
                if (is_allowed_scroll) {
                    scroll_lines(1);
                }
            } else {
                cur_row++;
            }
            logic_cursor = cur_row * CONSOLE_WIDTH;
            break;
        case '\r':
            logic_cursor = cur_row * CONSOLE_WIDTH;
            break;
        case '\b':
            logic_cursor--;
            if (cur_col == 0) {
                find_line_ending();
            } else {
                draw_char(cur_row, cur_col - 1, 0, term_color);
            }
            break;
        default:
            draw_char(cur_row, cur_col, s[i], term_color);
            if (logic_cursor == SCREEN_LEN - 1) {
                if (is_allowed_scroll) {
                    scroll_lines(1);
                    logic_cursor = SCREEN_LEN - CONSOLE_WIDTH;
                } else {
                    logic_cursor = 0;
                }
            } else {
                logic_cursor++;
            }
            break;
        }
    }

    get_cursor(&cur_row, &cur_col);
    set_cursor(cur_row, cur_col);
}

/**
 * @brief Prints character ch with the specified color at position (row, col).
 *        If any argument is invalid, the function has no effect.
 * @param row   the row in which to display the character.
 * @param col   the column in which to display the character.
 * @param ch    the character to display.
 * @param color the color to use to display the character.
 */
void draw_char(int row, int col, int ch, int color) {
    if (!is_pos_valid(row, col)) {
        return;
    }
    if (!is_color_valid(color)) {
        return;
    }

    char *char_addr =
        (char *)CONSOLE_MEM_BASE + 2 * (row * CONSOLE_WIDTH + col);
    *char_addr = ch;
    *(char_addr + 1) = color;
    return;
}

/**
 * @brief Returns the character displayed at position (row, col).
 * @param  row row of the character.
 * @param  col column of the character.
 * @return     the character at (row, col)
 */
char get_char(int row, int col) {
    if (!is_pos_valid(row, col)) {
        return 0xff;
    }

    char ch = *(char *)(CONSOLE_MEM_BASE + 2 * (row * CONSOLE_WIDTH + col));
    return ch;
}


/**
 * @brief Changes the foreground and background color of future characters
 *        printed on the console. If the color code is invalid,
 *        the function has no effect.
 * @param  color the new color code.
 * @return       [description]
 */
int set_term_color(int color) {
    if (!is_color_valid(color))
        return -1;

    term_color = color;
    return 0;
}

/**
 * @brief Writes the current foreground and background color of characters
 *        printed on the console into the argument color.
 * @param color The address to which the current color information
 *              will be written.
 */
void get_term_color(int *color) {
    *color = term_color;
}

/**
 * @brief Sets the position of the cursor to the position (row, col).
 *        Subsequent calls to putbyte or putbytes should cause the console
 *        output to begin at the new position.
 * @param  row the new row for the cursor.
 * @param  col the new column for the cursor.
 * @return     0 on success, or integer error code less than 0 if cursor
 *             location is invalid.
 */
int set_cursor(int row, int col) {
    if (!is_pos_valid(row, col)) {
        return -1;
    }

    logic_cursor = row * CONSOLE_WIDTH + col;
    if (!is_hide) {
        outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
        outb(CRTC_DATA_REG, (uint8_t)LSB(logic_cursor));

        outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
        outb(CRTC_DATA_REG, (uint8_t)MSB(logic_cursor));
    } else {
        outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
        outb(CRTC_DATA_REG, (uint8_t)LSB(CURSOR_POS_HIDE));

        outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
        outb(CRTC_DATA_REG, (uint8_t)MSB(CURSOR_POS_HIDE));
    }
    return 0;
}

/**
 * @brief Writes the current position of the cursor into the arguments
 *        row and col.
 * @param row the address to which the current cursor row will be written.
 * @param col the address to which the current cursor column will be written.
 */
void get_cursor( int *row, int *col ) {
    *row = logic_cursor / CONSOLE_WIDTH;
    *col = logic_cursor % CONSOLE_WIDTH;
}

/**
 * @brief Causes the cursor to become invisible. If the cursor is already
 *        invisible, the function has no effect.
 */
void hide_cursor() {
    if (is_hide)
        return;

    is_hide = 1;
    outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
    outb(CRTC_DATA_REG, (uint8_t)LSB(CURSOR_POS_HIDE));

    outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
    outb(CRTC_DATA_REG, (uint8_t)MSB(CURSOR_POS_HIDE));
}

/**
 * @brief Causes the cursor to become visible, without changing its location.
 *        If the cursor is already visible, the function has no effect.
 */
void show_cursor() {
    if (!is_hide)
        return;

    is_hide = 0;
    outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
    outb(CRTC_DATA_REG, logic_cursor);

    outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
    outb(CRTC_DATA_REG, logic_cursor >> 8);
}

/**
 * @brief Clears the entire console and resets the cursor to the home position
 *        (top left corner). If the cursor is currently hidden it should stay
 *        hidden.
 */
void clear_console() {
    int i = 0;
    int len = CONSOLE_WIDTH * CONSOLE_HEIGHT;
    for (i = 0; i < len; i++) {
        char *char_addr =
            (char *)(CONSOLE_MEM_BASE + 2 * i);
        *char_addr = 0;
        *(char_addr + 1) = term_color;
    }

    set_cursor(0, 0);
}

/**
 * @brief Check whether the color parameter is valid.
 * @param  color the color parameter
 * @return       0 for invalid, 1 for valid.
 */
int is_color_valid(int color) {
    unsigned int fgnd_mask = 0xf;
    unsigned int bgnd_mask = 0xf0;
    unsigned int fgnd_color = fgnd_mask & color;
    unsigned int bgnd_color = bgnd_mask & color >> 4;

    if (fgnd_color < 0x0 || fgnd_color > 0xF)
        return 0;
    if (bgnd_color < 0x0 || bgnd_color > 0x8)
        return 0;

    return 1;
}

/**
 * @brief Check whether the position parameter is within the screen range.
 * @param  row the row of position to be checked.
 * @param  col the coloum of position to be checked.
 * @return     0 for invalid, 1 for valid.
 */
int is_pos_valid(int row, int col) {
    if (row < 0 || row > CONSOLE_HEIGHT - 1 || \
            col < 0 || col > CONSOLE_WIDTH - 1)
        return 0;
    else
        return 1;
}

/**
 * @brief find the last character of the line where cursor is current located.
 *        this function is invoked when \b is entered when cursor is at new
 *        line, thus the cursor can return to the last character of the last
 *        line.
 */
void find_line_ending() {
    int line_ending = logic_cursor;
    int line_ending_boundry = logic_cursor / CONSOLE_WIDTH * CONSOLE_WIDTH;
    char *char_addr  = NULL;
    while (line_ending > line_ending_boundry) {
        char_addr = (char *)(CONSOLE_MEM_BASE + 2 * line_ending);
        if (*char_addr != 0)
            break;
        else
            line_ending--;
    }
    if (line_ending == logic_cursor) {
        int row;
        int col;
        get_cursor(&row, &col);
        draw_char(row, col, 0, term_color);
        return;
    }

    logic_cursor = *char_addr != 0 ? line_ending + 1 : line_ending;
}

/**
 * @brief Scroll the screen.
 * @param  num_line the number of lines that needs to be scrolled.
 * @return          0 for scroll successfully, -1 for fail.
 */
int scroll_lines(int num_line) {
    if (num_line > CONSOLE_HEIGHT) {
        return -1;
    } else if (num_line == CONSOLE_HEIGHT) {
        clear_console();
    } else {
        unsigned int offset = num_line * CONSOLE_WIDTH;
        int i;
        int screen_len = CONSOLE_WIDTH * CONSOLE_HEIGHT;
        /* move the entire array by num_line * CONSOLE_WIDTH */
        for (i = 0; i < screen_len - offset; i++) {
            char *char_addr_dst = (char *)(CONSOLE_MEM_BASE + 2 * i);
            char *char_addr_src = (char *)(CONSOLE_MEM_BASE + 2 * (i + offset));

            *char_addr_dst = *char_addr_src;
            *(char_addr_dst + 1) = *(char_addr_src + 1);
        }

        for (i = screen_len - offset; i < screen_len; i++) {
            char *char_addr = (char *)(CONSOLE_MEM_BASE + 2 * i);
            *char_addr = 0;
            * (char_addr + 1) = term_color;
        }
    }

    return 0;
}

/**
 * @brief
 * @param is_allowed [description]
 */
void set_is_allowed_scroll(int is_allowed) {
    is_allowed_scroll = is_allowed;
}
