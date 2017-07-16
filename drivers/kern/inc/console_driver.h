/**
 * @file console_driver.h
 * @brief A header file contains function declarations of console_driver.c
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_CONSOLE_DRIVER
#define H_CONSOLE_DRIVER

/* the lower 8 bit of address */
#define LSB(addr) ((addr) & 0x00ff)
/* the higher 8 bit of address */
#define MSB(addr) (((addr) & 0xff00) >> 8)
/* the location where is outside the screen range */
#define CURSOR_POS_HIDE (CONSOLE_WIDTH * CONSOLE_HEIGHT + 2)
#define SCREEN_LEN (CONSOLE_WIDTH * CONSOLE_HEIGHT)

/* function declarations */
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
int putbyte(char ch);

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
void putbytes(const char *s, int len);

/**
 * @brief Prints character ch with the specified color at position (row, col).
 *        If any argument is invalid, the function has no effect.
 * @param row   the row in which to display the character.
 * @param col   the column in which to display the character.
 * @param ch    the character to display.
 * @param color the color to use to display the character.
 */
void draw_char(int row, int col, int ch, int color);

/**
 * @brief Returns the character displayed at position (row, col).
 * @param  row row of the character.
 * @param  col column of the character.
 * @return     the character at (row, col)
 */
char get_char(int row, int col);

/**
 * @brief Changes the foreground and background color of future characters
 *        printed on the console. If the color code is invalid,
 *        the function has no effect.
 * @param  color the new color code.
 * @return       [description]
 */
int set_term_color(int color);

/**
 * @brief Writes the current foreground and background color of characters
 *        printed on the console into the argument color.
 * @param color The address to which the current color information
 *              will be written.
 */
void get_term_color(int *color);

/**
 * @brief Sets the position of the cursor to the position (row, col).
 *        Subsequent calls to putbyte or putbytes should cause the console
 *        output to begin at the new position.
 * @param  row the new row for the cursor.
 * @param  col the new column for the cursor.
 * @return     0 on success, or integer error code less than 0 if cursor
 *             location is invalid.
 */
int set_cursor(int row, int col);

/**
 * @brief Writes the current position of the cursor into the arguments
 *        row and col.
 * @param row the address to which the current cursor row will be written.
 * @param col the address to which the current cursor column will be written.
 */
void get_cursor(int *row, int *col);

/**
 * @brief Causes the cursor to become invisible. If the cursor is already
 *        invisible, the function has no effect.
 */
void hide_cursor();

/**
 * @brief Causes the cursor to become visible, without changing its location.
 *        If the cursor is already visible, the function has no effect.
 */
void show_cursor();

/**
 * @brief Clears the entire console and resets the cursor to the home position
 *        (top left corner). If the cursor is currently hidden it should stay
 *        hidden.
 */
void clear_console();

/**
 * @brief Check whether the color parameter is valid.
 * @param  color the color parameter
 * @return       0 for invalid, 1 for valid.
 */
int is_color_valid(int color);

/**
 * @brief Check whether the position parameter is within the screen range.
 * @param  row the row of position to be checked.
 * @param  col the coloum of position to be checked.
 * @return     0 for invalid, 1 for valid.
 */
int is_pos_valid(int row, int col);

/**
 * @brief find the last character of the line where cursor is current located.
 *        this function is invoked when \b is entered when cursor is at new
 *        line, thus the cursor can return to the last character of the last
 *        line.
 */
void find_line_ending();

/**
 * @brief Scroll the screen.
 * @param  num_line the number of lines that needs to be scrolled.
 * @return          0 for scroll successfully, -1 for fail.
 */
int scroll_lines(int num_line);

/**
 * @brief Control whether screen can be scrolled when the cursor reaches the end
 *        of screen.
 * @param is_allowed when set to 1, allow screen bo scrolled, otherwise, not
 *                   allowed
 */
void set_is_allowed_scroll(int is_allowed);

#endif /* H_CONSOLE_DRIVER */
