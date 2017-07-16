/**
 * @file game_ui.h
 * @brief A header file contains defines, function declarations.
 * @author Qiaoyu Deng
 * @andrew_id qdeng
 * @bug No known bug.
 */

#ifndef H_GAME_UI
#define H_GAME_UI

/* user define includes */
#include "inc/game.h"      /* struct best_record_t */
#include "video_defines.h" /* CONSOLE_HEIGHT */

#define BEST_RECORD_LEVEL_ROW   14
#define BEST_RECORD_LEVEL_COL   29
#define BEST_RECORD_TIME_ROW    14
#define BEST_RECORD_TIME_COL    45
#define FIRST_ROW_IN_SCREEN     0
#define INSTRCUTION_WIDTH       39
#define INSTRCUTION_HEIGHT      10
#define POP_UP_WINDOW_WIDTH     21
#define POP_UP_WINDOW_HEIGHT    5
#define SCORE_MAX_LENGTH        12
#define SCORE_POSITION_ROW      24
#define SCORE_POSITION_COLUMN   7
#define ATTEMPT_MAX_LENGTH      22
#define ATTEMPT_POSITION_ROW    24
#define ATTEMPT_POSITION_COLUMN 38
#define TIME_POSITION_ROW       24
#define TIME_POSITION_COLUMN    67

/* function declarations */
/**
 * @brief Print the game title.
 */
void print_title();

/**
 * @brief Print the best records according to current game_session.
 * @param best_record_ptr the pointer to the best_record_t structure
 */
void print_records(struct best_record_t *best_record_ptr);

/**
 * @brief Print the instruction window.
 */
void print_instruction();

/**
 * @brief Print the game layout base.
 */
void print_base();

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
                int *virus_row, int *virus_col);

/**
 * @brief Print the failed window.
 */
void print_failed();

/**
 * @brief Print the pass window.
 */
void print_pass();

/**
 * @brief Print the clear window
 */
void print_clear();

/**
 * @brief Print the pause window.
 */
void print_pause();

/**
 * @brief print the cheat window.
 */
void print_cheat();

/**
 * @brief Print the score of current game session.
 * @param score the score to be printed.
 */
void print_score(int score);

/**
 * @brief Print the number of attempt that player has tried to pass the game.
 * @param attempt the number to be printed.
 */
void print_attempt(int attemt);

/**
 * @brief Print the time of current level.
 * @param time_string the string representation of current level time.
 */
void print_time(char time_string[TIME_STRING_LENGTH + 1]);

/**
 * @brief Print window in the center of the screen.
 * @param win    the window to be printed.
 * @param width  the width of the window.
 * @param height the height of the window.
 */
void print_in_center(const char **win, int width, int height);

/**
 * @brief Print string in the center of the specific row.
 * @param str the string to be printed.
 * @param len the length of the string.
 * @param row the row to be printed.
 */
void print_in_line_center(char *str, int len, int row);

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
                      int current_height);

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
                          int *virus_row, int *virus_col);

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
                         int *ori_virus_row, int *ori_virus_col);

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
                     int *abs_row, int *abs_col);

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
               int src_row, int src_col, int dst_row, int dst_col);

#endif
