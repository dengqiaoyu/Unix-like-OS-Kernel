/**
 *  @file game.c
 *  @brief A kernel with timer, keyboard, console support
 *
 *  This file contains the kernel's main() function.
 *
 *  It sets up the drivers and starts the game.
 *
 *  @author Qiaoyu Deng
 *  @andrew_id qdeng
 *  @bug No known bugs.
 */

#include <p1kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */
#include <malloc.h>
#include <string.h>                 /* sprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* memory includes. */
#include <lmm.h>                    /* lmm_remove_free() */

/* x86 specific includes */
#include <x86/seg.h>                /* install_user_segs() */
#include <x86/interrupt_defines.h>  /* interrupt_setup() */
#include <x86/asm.h>                /* enable_interrupts() */

/* game map layout includes */
#include <flu_db.h>                 /* struct flu_maze and defines */

/* user driver and game ui includes */
#include "inc/install_driver.h"     /* handler_install() */
#include "inc/timer_driver.h"       /* cnt_seconds() */
#include "inc/console_driver.h"     /* operate cursor and putbyte() */
#include "inc/keyboard_driver.h"    /* readchar() */
#include "inc/game_ui.h"            /* game's UI */
#include "inc/game.h"


/* external globals */
extern const char *game_ui_title_win[CONSOLE_HEIGHT]; /* in game_ui.c */
extern const char *game_ui_main_layout_win[CONSOLE_HEIGHT]; /* in game_ui.c */
extern const flu_maze_t flu_mazes[]; /* in misc/flu_db.c */
extern const int flu_maze_count;     /* in misc/flu_db.c */
extern int if_cnt_time;              /* in timer_driver */

/* static globals */
/**
 * offset used when determining whether itis valid to go through that position
 */
static const int through_row_step_virus[MOVE_DIRECTION] = {0, 0, -1, 1};
static const int through_col_step_virus[MOVE_DIRECTION] = {1, -1, 0, 0};
/**
 * offset used when determining the final position of virus
 */
static const int expected_row_step_virus[MOVE_DIRECTION] = {
    0, 0, -VIRUS_STEP_SIZE, VIRUS_STEP_SIZE
};
static const int expected_col_step_virus[MOVE_DIRECTION] = {
    VIRUS_STEP_SIZE, -VIRUS_STEP_SIZE, 0, 0
};

/* static function */
static inline int abs(int x);
/* calculte the distance between two nodes without squaring and then rootting */
static inline int pseudo_distance(int x1, int y1, int x2, int y2);

/**
 * @brief Kernel entrypoint.
 *
 * This is the entrypoint for the kernel. It intializes timer driver and
 * keyboard driver, and then starts the game.
 * @param  mbinfo No
 * @param  argc   No
 * @param  argv   No
 * @param  envp   No
 * @return        Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    /*
     * Initialize device-driver library and send timer callback function.
     */
    handler_install(cnt_seconds);

    /*
     * Initialize game_session which hold the required information of game.
     */
    struct game_session_t game_session;
    memset(&game_session, 0, sizeof(struct game_session_t));
    /* At first, there is no records in the game. */
    init_records(&game_session);

    /*
     * Main routine for the game, and it loops forever.
     */
    while (1) {
        init_game_session(TITLE, &game_session);
        game_title(&game_session);
        game_main(&game_session);
    }
    return 0;
}

/**
 * @brief Game initial function for the first running.
 *
 * the position of both player and virus is set to -1 indicating that they
 * remain in the original position in the map layout.
 *
 * @param status       the current status of user at.
 * @param game_session the pointer to the game_session_t variable.
 */
void init_game_session(int status, struct game_session_t *game_session) {
    game_session->player_row = INITIAL_POSITION;
    game_session->player_col = INITIAL_POSITION;
    game_session->virus_row = INITIAL_POSITION;
    game_session->virus_col = INITIAL_POSITION;
    game_session->level = INITIAL_LEVEL;
    game_session->score = 0;
    game_session->attempt = 0;
    game_session->status = status;
}

/**
 * @brief Initial the record of game.
 *
 * It uses link list to create MAZE_RECENT_RECORDS_NUM items with a first
 * dumming head(NULL item).
 *
 * @param game_session the pointer to the game_session_t variable.
 */
void init_records(struct game_session_t *game_session) {
    int i;
    struct best_record_t *rover = &(game_session->best_record);
    for (i = 0; i < MAZE_RECENT_RECORDS_NUM; i++) {
        struct best_record_t *new_record =
            calloc(1, sizeof(struct best_record_t));
        new_record->level = INITIAL_RECORD;
        snprintf(new_record->level_str, LEVEL_STRING_LENGTH + 1,
                 "No record");
        new_record->second = 0;
        snprintf(new_record->time_str, TIME_STRING_LENGTH + 1, "00h 00m 00s");
        rover->next = new_record;
        rover = rover->next;
    }
}

/**
 * @brief Display the title window of game.
 *
 * It wait for user's key press to start game.
 *
 * @param game_session the pointer to the game_session_t variable.
 */
void game_title(struct game_session_t *game_session) {
    /*
     * Since we need to display a game, all features related to text editing
     * should be disabled.
     */
    set_is_allowed_scroll(0);
    hide_cursor();

    print_title();
    print_records(&game_session->best_record);
    while (1) {
        int is_break = 0; /* used for check whether to start the game. */
        char ch = readchar();
        switch (game_session->status) {
        case TITLE: /* user is at title window. */
            if (ch == 's') {
                game_session->status = MAIN;
                is_break = 1;
            } else if (ch == 'i') {
                game_session->status = INSTRUCTION_IN_TITLE;
                print_instruction();
            }
            break;
        /*
         * user is at instruction window over the title window.
         */
        case INSTRUCTION_IN_TITLE:
            if (ch == 'i') {
                game_session->status = TITLE;
                restore_prev_win(game_ui_title_win,
                                 CONSOLE_WIDTH,
                                 CONSOLE_HEIGHT,
                                 INSTRCUTION_WIDTH,
                                 INSTRCUTION_HEIGHT);
                print_records(&game_session->best_record);
            }
            break;
        default:
            game_session->status = TITLE;
            print_title();
            break;
        }
        if (is_break != 0)
            break;
    }
}


/**
 * @brief Main function for running game.
 *
 * It receives key press to running the game and print different game UI.
 *
 * @param game_session the pointer to the game_session_t variable.
 */
void game_main(struct game_session_t *game_session) {
    game_session->session_start_time = get_seconds();
    /* store whether user has press 'v' to pass game. */
    int is_cheat = 0;
    while (1) {
        /* store whether user has press 'q' to quit the current game. */
        int is_quit = 0;
        game_session->attempt++;
        print_base();
        display_main_layout(game_session);
        /* start to count time. */
        game_session->level_start_time = get_seconds();
        while (1) {
            /* store whether player want to skip th ecurrent level. */
            int is_pass = 0;
            char ch = readchar();
            /* If player is at the normal window to playe game. */
            if (game_session->status == MAIN) {
                switch (ch) {
                /*
                 * capture all of move instructions.
                 */
                case 'w':
                case 'a':
                case 's':
                case 'd': {
                    int is_arrived = 0;
                    is_arrived = move_player(ch, game_session);
                    /* user has arrived the destination. */
                    if (is_arrived == 1) {
                        if_cnt_time = 0; /* stop counting time */
                        game_session->score++;
                        print_score(game_session->score);
                        /* get total time of clearing this level. */
                        unsigned int total_time =
                            get_seconds() - game_session->level_start_time;
                        replace_new_record(game_session->level, total_time,
                                           &game_session->best_record);
                        /* if player have passed all the levels */
                        if (game_session->level == flu_maze_count) {
                            /* check if player ever used skip level to cheat */
                            if (is_cheat) {
                                game_session->status = CHEAT_IN_MAIN;
                                print_cheat();
                                break;
                            }
                            game_session->status = CLEAR_IN_MAIN;
                            print_clear();
                            break;
                        }
                        game_session->status = PASS_IN_MAIN;
                        print_pass();
                        break;
                    } else if (is_arrived == -1) {
                        /* the move on this direction is invalid. */
                        continue;
                    }
                    /* FALLTHROUGH */
                }
                /*
                 * player press [space] key to skip current turn, or fall from
                 * last section.
                 */
                case ' ': {
                    int is_catched = 0;
                    /* only move virus. */
                    is_catched = move_virus(game_session);
                    if (is_catched != 0) {
                        if_cnt_time = 0;
                        game_session->status = CATCHED_IN_MAIN;
                        print_failed();
                    }
                    break;
                }
                /*
                 * player press 'r' key to restart the current level.
                 */
                case 'r':
                    restore_ori_position(game_session->level,
                                         &game_session->player_row,
                                         &game_session->player_col,
                                         &game_session->virus_row,
                                         &game_session->virus_col);
                    game_session->attempt++;
                    print_attempt(game_session->attempt);
                    break;
                /*
                 * player press 'p' key to pause the game, and the layout is
                 * hided not allowing player to think.
                 */
                case 'p':
                    game_session->status = PAUSE_IN_MAIN;
                    if_cnt_time = 0;
                    print_pause();
                    break;
                /*
                 * player press 'q' key to quit current game session.
                 */
                case 'q':
                    is_quit = 1;
                    break;
                /*
                 * player press 'i' key to to toggle operating direction.
                 */
                case 'i':
                    game_session->status = INSTRUCTION_IN_MAIN;
                    print_instruction();
                    break;
                /*
                 * player press 'v' key to directly skip to next level.
                 */
                case 'v':
                    is_cheat = 0;
                    set_to_next_level(game_session);
                    is_pass = 1;
                    break;
                default:
                    break;
                }
                if (is_quit != 0) {
                    game_session->status = TITLE;
                    break;
                }
            } else if (game_session->status == PAUSE_IN_MAIN && ch == 'p') {
                /*
                 * if player has paused the game, and press 'p' to continue.
                 */
                game_session->status = MAIN;
                if_cnt_time = 1; /* continue counting time. */
                print_base();
                display_main_layout(game_session);
            } else if (game_session->status == INSTRUCTION_IN_MAIN \
                       && ch == 'i') {
                /*
                 * if player triggered instruction window, and press 'i' to
                 * return game.
                 */
                game_session->status = MAIN;
                restore_prev_win(game_ui_main_layout_win,
                                 CONSOLE_WIDTH,
                                 CONSOLE_HEIGHT,
                                 INSTRCUTION_WIDTH,
                                 INSTRCUTION_HEIGHT);
                display_main_layout(game_session);
            } else if (game_session->status == CATCHED_IN_MAIN && ch == 'r') {
                /*
                 * if player is catched by the virus, he/she is required to
                 * press 'r' to restart the current level.
                 */
                game_session->status = MAIN;
                restore_prev_win(game_ui_main_layout_win,
                                 CONSOLE_WIDTH,
                                 CONSOLE_HEIGHT,
                                 INSTRCUTION_WIDTH,
                                 INSTRCUTION_HEIGHT);
                game_session->player_row = INITIAL_POSITION;
                game_session->player_col = INITIAL_POSITION;
                game_session->virus_row = INITIAL_POSITION;
                game_session->virus_col = INITIAL_POSITION;
                display_main_layout(game_session);
                game_session->attempt++;
                print_attempt(game_session->attempt);
                /* continue counting time. */
                if_cnt_time = 1;
            } else if (game_session->status == PASS_IN_MAIN && ch == 'v') {
                /*
                 * if player has arrived the destination before captured by
                 * virus.
                 */
                set_to_next_level(game_session);
                is_pass = 1;
                /* continue counting time. */
                if_cnt_time = 1;
            } else if (game_session->status == CLEAR_IN_MAIN && ch == 'r') {
                /*
                 * if player has arrived the destination before captured by
                 * virus.
                 */
                init_game_session(MAIN, game_session);
                is_pass = 1;
                /* continue counting time. */
                if_cnt_time = 1;
            } else if (game_session->status == CHEAT_IN_MAIN && ch == 'r') {
                /*
                 * if player has passed all the levels but at least he/she has
                 * use v to skip the game once.
                 */
                init_game_session(MAIN, game_session);
                is_pass = 1;
                /* continue counting time. */
                if_cnt_time = 1;
            }

            /* update the timer */
            refresh_time(game_session->level_start_time);
            if (is_pass != 0)
                break;
        }
        if (is_quit != 0)
            break;
    }
}

/**
 * @brief Display the main interface of game.
 * @param game_session the pointer to the game_session_t variable.
 */
void display_main_layout(struct game_session_t *game_session) {
    print_maze(game_session->level,
               &game_session->player_row,
               &game_session->player_col,
               &game_session->virus_row,
               &game_session->virus_col);
    print_score(game_session->score);
    print_attempt(game_session->attempt);
}

/**
 * @brief Update timer.
 * @param start_time the start time of current level.
 */
void refresh_time(int start_time) {
    if (start_time == get_seconds())
        return;
    int time_diff = get_seconds() - start_time;
    char time_string[TIME_STRING_LENGTH + 1] = {0};
    second_to_time_str(time_string, time_diff);
    print_time(time_string);
}

/**
 * @brief convert second value to string representation.
 * @param time_string string pointer.
 * @param time second value.
 */
void second_to_time_str(char time_string[TIME_STRING_LENGTH + 1],
                        unsigned int time) {
    unsigned int hours = time / SECONDS_PER_HOUR;
    unsigned int mins = (time - hours * SECONDS_PER_HOUR) / SECONDS_PER_MINUTES;
    unsigned int seconds =
        (time - hours * SECONDS_PER_HOUR - mins * SECONDS_PER_MINUTES);

    snprintf(time_string, TIME_STRING_LENGTH + 1, "%02dh %02dm %02ds",
             hours, mins, seconds);
}

/**
 * @brief move the player in the maze if valid.
 * @param  direction    up, down, left or right.
 * @param  game_session the pointer to the game_session_t variable.
 * @return              0 if current move can be performed, -1 if cannot,
 *                      1 if player has moved to destination.
 */
int move_player(char direction, struct game_session_t *game_session) {
    int player_row = game_session->player_row;
    int player_col = game_session->player_col;
    /* the adjcent item inside the maze, if it is [space], current move
     * can be performed.
     */
    int through_row = player_row;
    int through_col = player_col;
    /* if current move can be performed, the expected new position of player. */
    int expected_row = player_row;
    int expected_col = player_col;
    flu_maze_t const *flu_maze_ptr = &flu_mazes[game_session->level - 1];
    const char **maze = (const char **)flu_maze_ptr->maze;
    switch (direction) {
    case 'w':
        through_row--;
        expected_row -= PLAYER_STEP_SIZE;
        break;
    case 'a':
        through_col--;
        expected_col -= PLAYER_STEP_SIZE;
        break;
    case 's':
        through_row++;
        expected_row += PLAYER_STEP_SIZE;
        break;
    case 'd':
        through_col++;
        expected_col += PLAYER_STEP_SIZE;
        break;
    default:
        return 0;
    }
    char maze_item = maze[through_row][through_col];
    if (maze_item == ' ') {
        game_session->player_row = expected_row;
        game_session->player_col = expected_col;
        move_item('@', flu_maze_ptr->width, flu_maze_ptr->height,
                  player_row, player_col,
                  expected_row, expected_col);
        if (maze[expected_row][expected_col] == 'H') {
            return 1;
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

/**
 * @brief move the virus in the maze 2 steps if it can get closer to the player.
 * @param  game_session the pointer to the game_session_t variable.
 * @return              0 for not catching up player, 1 for catching up player.
 */
int move_virus(struct game_session_t *game_session) {
    flu_maze_t const *flu_maze_ptr = &flu_mazes[game_session->level - 1];
    const char **maze = (const char **)flu_maze_ptr->maze;
    int player_row = game_session->player_row;
    int player_col = game_session->player_col;
    int is_move = 0;
    int i;
    for (i = 0; i < TURNS_PER_MOVE; i++) {
        int virus_row = game_session->virus_row;
        int virus_col = game_session->virus_col;
        if (player_row == virus_row && player_col == virus_col) {
            move_item('V', flu_maze_ptr->width, flu_maze_ptr->height,
                      virus_row, virus_col,
                      player_row, player_col);
            return 1;
        }

        int through_row;
        int through_col;
        int expected_row;
        int expected_col;
        int j;
        /* j = 0, 1, 2, 3 represents moving RIGHT, LEFT, UP and DOWN */
        for (j = 0; j < MOVE_DIRECTION; j++) {
            through_row = virus_row + through_row_step_virus[j];
            through_col = virus_col + through_col_step_virus[j];
            expected_row = virus_row + expected_row_step_virus[j];
            expected_col = virus_col + expected_col_step_virus[j];
            is_move = check_is_move(maze, through_row, through_col,
                                    player_row, player_col,
                                    virus_row , virus_col,
                                    expected_row, expected_col);
            if (is_move != 0) {
                game_session->virus_row = expected_row;
                game_session->virus_col = expected_col;
                move_item('V', flu_maze_ptr->width, flu_maze_ptr->height,
                          virus_row, virus_col,
                          expected_row, expected_col);
                if (expected_row == player_row && expected_col == player_col) {
                    return 1;
                }
                break;
            }
        }
    }
    return 0;
}

/**
 * @brief Check whether current move method can make virus closer to player.
 * @param  maze         string array that store the current level layout.
 * @param  through_row  the row of adjacent item
 * @param  through_col  the column of adjacent item
 * @param  player_row   the row of player
 * @param  player_col   the column of player
 * @param  virus_row    the row of virus
 * @param  virus_col    the column of virus
 * @param  expected_row the expected row of virus if it can perform that move
 * @param  expected_col the expected column of virus if it can perform that move
 * @return              0 for not moving, 1 for moving.
 */
int check_is_move(const char **maze,
                  unsigned int through_row, unsigned int through_col,
                  unsigned int player_row, unsigned int player_col,
                  unsigned int virus_row , unsigned int virus_col,
                  unsigned int expected_row, unsigned int expected_col) {
    if (maze[through_row][through_col] != ' ')
        return 0;
    int ori_distance = pseudo_distance(player_row, player_col,
                                       virus_row, virus_col);
    int expected_distance = pseudo_distance(player_row, player_col,
                                            expected_row, expected_col);
    if (expected_distance < ori_distance)
        return 1;
    else
        return 0;
}

/**
 * @brief Set game_session for next new game level.
 * @param game_session [description]
 */
void set_to_next_level(struct game_session_t *game_session) {
    game_session->level++;
    game_session->attempt = 0;
    game_session->status = MAIN;
    /* -1 means that player and virus is in the original position */
    game_session->player_row = -1;
    game_session->player_col = -1;
    game_session->virus_row = -1;
    game_session->virus_col = -1;
}

/**
 * Replace the old record with new one.
 * @param level       the game level to be replaced.
 * @param total_time  total time for passing that level.
 * @param best_record pointer to best_record variable.
 */
void replace_new_record(int level, int total_time,
                        struct best_record_t *best_record) {
    struct best_record_t *prev = best_record;
    struct best_record_t *next = prev->next;
    int i;
    int is_add_new = 1;
    int is_delete_last = 1;
    for (i = 0; i < MAZE_RECENT_RECORDS_NUM; i++) {
        /* if an old record has existed for the same level, replace that one. */
        if (next->level == level) {
            if (total_time <= next->second) {
                prev->next = next->next;
                free(next);
                is_delete_last = 0;
            } else {
                is_add_new = 0;
                is_delete_last = 0;
            }
            break;
        }
        prev = next;
        next = next->next;
    }
    if (is_add_new) {
        struct best_record_t *new_record =
            calloc(1, sizeof(struct best_record_t));
        new_record->level = level;
        new_record->second = total_time;
        snprintf(new_record->level_str, LEVEL_STRING_LENGTH + 1,
                 "Level %d", level);
        second_to_time_str(new_record->time_str, total_time);
        new_record->next = best_record->next;
        best_record->next = new_record;

        /* Delete the last remaining record. */
        if (is_delete_last) {
            next = best_record;
            for (i = 0; i < MAZE_RECENT_RECORDS_NUM; i++) {
                next = next->next;
            }
            free(next->next);
            next->next = NULL;
        }
    }
}

int abs(int x) {
    return x < 0 ? -x : x;
}

int pseudo_distance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}
