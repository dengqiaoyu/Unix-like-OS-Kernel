/**
 * @brief This file contains different helper function to print interface and
 *        layout
 * @author Qaioyu Deng
 * @andrew_id qdeng
 * @bug No known bug.
 */

/* libc includes */
#include <stdio.h>                  /* printf() */
#include <string.h>                 /* strlen() */
#include <stdio.h>                  /* snprintf() */

/* debug include */
#include <simics.h>                 /* lprintf() */

/* game map layout include */
#include <flu_db.h>                 /* maze */

/* user define includes */
#include "video_defines.h"          /* CONSOLE_HEIGHT */
#include "inc/console_driver.h"     /* clear_console(), set_is_allowed_scroll */
#include "inc/game.h"               /* struct best_record */
#include "inc/game_ui.h"


/* external globals */
extern const int flu_maze_count;
extern const flu_maze_t flu_mazes[];

/* const defines */
const char *game_ui_title_win[CONSOLE_HEIGHT] = {
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                              Escape the Flu Virus                     \n",
    "                                                                       \n",
    "                               Press 's' to start                      \n",
    "                        Press 'i' to toggle instructions               \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                Rencent Records                        \n",
    "                         1. [              ] 00h 00m 00s               \n",
    "                         2. [              ] 00h 00m 00s               \n",
    "                         3. [              ] 00h 00m 00s               \n",
    "                         4. [              ] 00h 00m 00s               \n",
    "                         5. [              ] 00h 00m 00s               \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                   By Qiaoyu                           \n",
    "                 Inspired by Robert Abbott's 'Theseus and the Minotaur'\n",
    "                                                                       \n"
};

/* It has some lines greater than 80 columns */
const char *game_ui_main_layout_win[CONSOLE_HEIGHT] = {
    "                                [              ]          \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                                       \n",
    "                                                              'i' to toggle\n",
    "                                                                  instructions\n",
    "                                                                       \n",
    "                                                                       \n",
    "Score:                        Attempt:                       Time: 00h 00m 00s",
};

const char *instruction_win[INSTRCUTION_HEIGHT] = {
    "***************************************",
    "*           'wasd' to move            *",
    "*           'space' to wait           *",
    "*            'r' to rewind            *",
    "*             'q' to quit             *",
    "*        'p' to pause/unpause         *",
    "*     'i' to toggle instructions      *",
    "*      'v' to skip current level      *",
    "*         Press 'i' to go back        *",
    "***************************************"
};

const char *failed_win[POP_UP_WINDOW_HEIGHT] = {
    "*********************",
    "*     So bad! :(    *",
    "*  You are killed!  *",
    "*  'r' to restart   *",
    "*********************"
};

const char *passed_win[POP_UP_WINDOW_HEIGHT] = {
    "*********************",
    "*  Congradulations  *",
    "*   You are cured   *",
    "* 'v' to next stage *",
    "*********************"
};

const char *cleared_win[POP_UP_WINDOW_HEIGHT] = {
    "*********************",
    "*  El Psy Congroo!  *",
    "*  You cleard all   *",
    "*  'r' to restart   *",
    "*********************"
};

const char *cheated_win[POP_UP_WINDOW_HEIGHT] = {
    "*********************",
    "*  You cleard all   *",
    "* But you cheated!  *",
    "*  'r' to restart   *",
    "*********************"
};

const char *pause_win[CONSOLE_HEIGHT - 1] = {
    "How about taking a rest, but... you cannot cheat!                      \n",
    "                                                  /NMMNm-              \n",
    "Press 'p' to resume                                .dMMMMo             \n",
    "                                                     yMMMMh`           \n",
    "                                                      oMMMMd`          \n",
    "                                                       sMMMMd`         \n",
    "               ohhhhh`                                  hMMMMy         \n",
    "               hMMMMM.                                  .NMMMM:        \n",
    "               hMMMMM.                                   sMMMMd        \n",
    "               -:::::`                                   -MMMMM`       \n",
    "                                                          MMMMM:       \n",
    "                             :ddddddddddddds              mMMMM+       \n",
    "                             / MMMMMMMMMMMMMh              mMMMM +     \n",
    "                             -yyyyyyyyyyyyy+              NMMMM/       \n",
    "                                                         `MMMMM -      \n",
    "               .-----                                    /MMMMN        \n",
    "               hMMMMM.                                   dMMMMs        \n",
    "               hMMMMM.                                  /MMMMN`        \n",
    "               ohhMMM.                                 .NMMMM /        \n",
    "                 :MMN                                 `mMMMM+          \n",
    "                `dMMo                                .mMMMM +                  \n",
    "               `dMMh                                :NMMMN:                    \n",
    "               .-- -                                oMMMMd.            \n",
    "                                                  :oooo+               \n"
};


/* function definitions */
/**
 * @brief Print the game title.
 */
void print_title() {
    clear_console();
    int i;
    for (i = 0; i < CONSOLE_HEIGHT; i++) {
        putbytes(game_ui_title_win[i], strlen(game_ui_title_win[i]));
    }
}

/**
 * @brief Print the best records according to current game_session.
 * @param best_record_ptr the pointer to the best_record_t structure
 */
void print_records(struct best_record_t *best_record_ptr) {
    int i;
    struct best_record_t *rover = best_record_ptr;
    for (i = 0; i < MAZE_RECENT_RECORDS_NUM; i++) {
        rover = rover->next;
        int best_record_col_offset =
            (LEVEL_STRING_LENGTH - strlen(rover->level_str)) / 2;
        set_cursor(BEST_RECORD_LEVEL_ROW + i,
                   BEST_RECORD_LEVEL_COL + best_record_col_offset);
        putbytes(rover->level_str, strlen(rover->level_str));
        set_cursor(BEST_RECORD_TIME_ROW + i,
                   BEST_RECORD_TIME_COL);
        putbytes(rover->time_str, TIME_STRING_LENGTH);
    }
}

/**
 * @brief Print the instruction window.
 */
void print_instruction() {
    print_in_center(instruction_win, INSTRCUTION_WIDTH, INSTRCUTION_HEIGHT);
}

/**
 * @brief Print the game layout base.
 */
void print_base() {
    clear_console();
    int i;
    for (i = 0; i < CONSOLE_HEIGHT; i++) {
        putbytes(game_ui_main_layout_win[i], strlen(game_ui_main_layout_win[i]));
    }
}

/**
 * @brief Print the maze layout based on the player and virus's position.
 * @param level      game level that should be specificed to indicate which maze
 *                   to use.
 * @param player_row the relative row of player inside the maze, or
 *                   INITIAL_POSITION(-1).
 * @param player_col the relative column of player inside the maze, or
 *                   INITIAL_POSITION(-1).
 * @param virus_row  the relative row of virus inside the maze, or
 *                   INITIAL_POSITION(-1).
 * @param virus_col  the relative column of virus inside the maze, or
 *                   INITIAL_POSITION(-1).
 */
void print_maze(int level, int *player_row, int *player_col,
                int *virus_row, int *virus_col) {
    if (level > flu_maze_count || level < 0)
        level = 1;
    const flu_maze_t *current_maze = &flu_mazes[level - 1];
    char level_str[LEVEL_STRING_LENGTH + 1] = {0};
    snprintf(level_str, LEVEL_STRING_LENGTH + 1, "Level %d", level);
    /* print level name */
    print_in_line_center(level_str, strlen(level_str), FIRST_ROW_IN_SCREEN);
    /* print layout */
    print_in_center((const char **)current_maze->maze,
                    current_maze->width, current_maze->height);
    int ori_player_row;
    int ori_player_col;
    int ori_virus_row;
    int ori_virus_col;
    search_player_virus((const char **)current_maze->maze,
                        current_maze->width, current_maze->height,
                        &ori_player_row, &ori_player_col,
                        &ori_virus_row, &ori_virus_col);
    /* if player and virus is at the original position */
    if (*player_col == INITIAL_POSITION) {
        *player_row = ori_player_row;
        *player_col = ori_player_col;
        *virus_row = ori_virus_row;
        *virus_col = ori_virus_col;
    } else { /* restore the player and virus's position */
        move_item('@', current_maze->width, current_maze->height,
                  ori_player_row, ori_player_col,
                  *player_row, *player_col);
        move_item('V', current_maze->width, current_maze->height,
                  ori_virus_row, ori_virus_col,
                  *virus_row, *virus_col);
    }
}

/**
 * @brief Print the failed window.
 */
void print_failed() {
    print_in_center(failed_win, POP_UP_WINDOW_WIDTH, POP_UP_WINDOW_HEIGHT);
}

/**
 * @brief Print the pass window.
 */
void print_pass() {
    print_in_center(passed_win, POP_UP_WINDOW_WIDTH, POP_UP_WINDOW_HEIGHT);
}

/**
 * @brief Print the clear window
 */
void print_clear() {
    print_in_center(cleared_win, POP_UP_WINDOW_WIDTH, POP_UP_WINDOW_HEIGHT);
}

/**
 * @brief Print the pause window.
 */
void print_pause() {
    set_cursor(0, 0);
    int i;
    for (i = 0; i < CONSOLE_HEIGHT - 1; i++) {
        putbytes(pause_win[i], strlen(pause_win[i]));
    }
}

/**
 * @brief print the cheat window.
 */
void print_cheat() {
    print_in_center(cheated_win, POP_UP_WINDOW_WIDTH, POP_UP_WINDOW_HEIGHT);
}

/**
 * @brief Print the score of current game session.
 * @param score the score to be printed.
 */
void print_score(int score) {
    set_cursor(SCORE_POSITION_ROW, SCORE_POSITION_COLUMN);
    char score_str[SCORE_MAX_LENGTH + 1] = "            \0";
    putbytes(score_str, SCORE_MAX_LENGTH);

    snprintf(score_str, SCORE_MAX_LENGTH + 1, "%d", score);
    set_cursor(SCORE_POSITION_ROW, SCORE_POSITION_COLUMN);
    putbytes(score_str, SCORE_MAX_LENGTH);
}

/**
 * @brief Print the number of attempt that player has tried to pass the game.
 * @param attempt the number to be printed.
 */
void print_attempt(int attempt) {
    set_cursor(ATTEMPT_POSITION_ROW, ATTEMPT_POSITION_COLUMN);
    char attempt_str[ATTEMPT_MAX_LENGTH + 1] = "                      \0";
    putbytes(attempt_str, ATTEMPT_MAX_LENGTH);

    snprintf(attempt_str, ATTEMPT_MAX_LENGTH + 1, "%d", attempt);
    set_cursor(ATTEMPT_POSITION_ROW, ATTEMPT_POSITION_COLUMN);
    putbytes(attempt_str, ATTEMPT_MAX_LENGTH);
}

/**
 * @brief Print the time of current level.
 * @param time_string the string representation of current level time.
 */
void print_time(char time_string[TIME_STRING_LENGTH + 1]) {
    set_cursor(TIME_POSITION_ROW, TIME_POSITION_COLUMN);
    putbytes(time_string, TIME_STRING_LENGTH);
}

/**
 * @brief Print window in the center of the screen.
 * @param win    the window to be printed.
 * @param width  the width of the window.
 * @param height the height of the window.
 */
void print_in_center(const char *win[], int width, int height) {
    int offset_row = (CONSOLE_HEIGHT - height) / 2;
    int offset_col = (CONSOLE_WIDTH - width) / 2;
    int i;
    for (i = 0; i < height; i++) {
        set_cursor(offset_row + i, offset_col);
        putbytes(win[i], width);
    }
}

/**
 * @brief Print string in the center of the specific row.
 * @param str the string to be printed.
 * @param len the length of the string.
 * @param row the row to be printed.
 */
void print_in_line_center(char *str, int len, int row) {
    char *str_too_long = "STRING TO BE PRINTED TOO LONG";
    char *valid_str = str;
    if (len > CONSOLE_WIDTH || len < 0) {
        valid_str = str_too_long;
        len = strlen(str_too_long);
    }
    int offset_row = row;
    int offset_col = (CONSOLE_WIDTH - len) / 2;

    set_cursor(offset_row, offset_col);
    putbytes(valid_str, len);
}

/**
 * @brief redisplay the previous window which is cover partially by the current
 *        window.
 * @param prev_win       the pointer of the previous window.
 * @param prev_width     the width of the previous window.
 * @param prev_height    the height of the previous window.
 * @param current_width  the height of current window.
 * @param current_height the width of current window.
 */
void restore_prev_win(const char **prev_win,
                      int prev_width,
                      int prev_height,
                      int current_width,
                      int current_height) {
    if (!(prev_width > current_width && prev_height > current_height))
        return;
    int offset_row = (prev_height - current_height) / 2;
    int offset_col = (prev_width - current_width) / 2;
    int i;
    for (i = 0; i < current_height; i++) {
        set_cursor(offset_row + i, offset_col);
        putbytes(&prev_win[offset_row + i][offset_col], current_width);
    }
}

/**
 * @brief Reset the player and virus's position to original setting.
 * @param level      the game level of curent game layout.
 * @param player_row the pointer of player_row in game_session.
 * @param player_col the pointer of player_col in game_session.
 * @param virus_row  the pointer of virus_row in game_session.
 * @param virus_col  the pointer of virus_col in game_session.
 */
void restore_ori_position(int level,
                          int *player_row, int *player_col,
                          int *virus_row, int *virus_col) {
    int ori_player_row = 0;
    int ori_player_col = 0;
    int ori_virus_row = 0;
    int ori_virus_col = 0;
    const flu_maze_t *current_maze = &flu_mazes[level - 1];
    int width = current_maze->width;
    int height = current_maze->height;

    /* search the position of virus and player in the original maze */
    // TODO Can be improved by storing thier original position when start at the
    //      first time.
    search_player_virus((const char**)current_maze->maze, width, height,
                        &ori_player_row, &ori_player_col,
                        &ori_virus_row, &ori_virus_col);
    move_item('@', width, height, *player_row, *player_col,
              ori_player_row, ori_player_col);
    move_item('V', width, height, *virus_row, *virus_col,
              ori_virus_row, ori_virus_col);
    *player_row = ori_player_row;
    *player_col = ori_player_col;
    *virus_row = ori_virus_row;
    *virus_col = ori_virus_col;
}

/**
 * @brief Search the positions of player and virus inside the maze.
 * @param maze           the pointer to the maze.
 * @param width          the width of the maze.
 * @param height         the height of the maze.
 * @param ori_player_row the pointer to be used to store the results.
 * @param ori_player_col the pointer to be used to store the results.
 * @param ori_virus_row  the pointer to be used to store the results.
 * @param ori_virus_col  the pointer to be used to store the results.
 */
void search_player_virus(const char *maze[MAZE_MAX_HEIGHT],
                         int width, int height,
                         int *ori_player_row, int *ori_player_col,
                         int *ori_virus_row, int *ori_virus_col) {
    int i;
    int j;
    for (i = 0; i < height; i ++) {
        for (j = 0; j < width; j++) {
            if (maze[i][j] == '@') {
                *ori_player_row = i;
                *ori_player_col = j;
            } else if (maze[i][j] == 'V') {
                *ori_virus_row = i;
                *ori_virus_col = j;
            }
        }
    }
}

/**
 * @brief Convert relative position in the maze to the position in the screen.
 * @param width        the width if the maze.
 * @param height       the height of the maze.
 * @param relative_row the relative row of the item in the maze.
 * @param relative_col the relative column of the item in the maze.
 * @param abs_row      the absolute row of the item in the maze.
 * @param abs_col      the absolute column of the item in the maze.
 */
void relative_to_abs(int width, int height,
                     int relative_row, int relative_col,
                     int *abs_row, int *abs_col) {
    int offset_row = (CONSOLE_HEIGHT - height) / 2;
    int offset_col = (CONSOLE_WIDTH - width) / 2;
    *abs_row = relative_row + offset_row;
    *abs_col = relative_col + offset_col;
}

/**
 * @brief Move the item from relative source position to relative destination
 *        position.
 * @param ch      The charachter that needs to be printed at new position.
 * @param width   the width of the maze.
 * @param height  the height of the maze.
 * @param src_row the relative source row of the item in the maze.
 * @param src_col the relative source column of the item in the maze.
 * @param dst_row the relative destination row of the item in the maze.
 * @param dst_col the relative destination column of the item in the maze.
 */
void move_item(char ch, int width, int height,
               int src_row, int src_col, int dst_row, int dst_col) {
    int abs_src_row = 0;
    int abs_src_col = 0;
    int abs_dst_row = 0;
    int abs_dst_col = 0;

    relative_to_abs(width, height, src_row, src_col,
                    &abs_src_row, &abs_src_col);
    relative_to_abs(width, height, dst_row, dst_col,
                    &abs_dst_row, &abs_dst_col);

    set_cursor(abs_src_row, abs_src_col);
    putbyte('.');
    set_cursor(abs_dst_row, abs_dst_col);
    putbyte(ch);
}
