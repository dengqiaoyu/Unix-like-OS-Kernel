/**
 * @file game.h
 * @brief A header file contains main structures, function declarations
 * @author Qioayu Deng
 * @andrew_id qdeng
 * @bug No known bugs.
 */

#ifndef H_GAME
#define H_GAME

/* string length limitation */
#define MAZE_RECENT_RECORDS_NUM (5)
#define LEVEL_STRING_LENGTH     (14)
#define LEVEL_BRACKET_LENGTH    (2)
#define TIME_STRING_LENGTH      (11)

/* move helper defines */
#define TURNS_PER_MOVE   (2)
#define MOVE_DIRECTION   (4)
#define PLAYER_STEP_SIZE (2)
#define VIRUS_STEP_SIZE  (2)

/* defines for time convertion */
#define SECONDS_PER_HOUR    (3600)
#define SECONDS_PER_MINUTES (60)

/* defines for game_session initial values */
#define INITIAL_LEVEL     (1)
#define INITIAL_POSITION (-1)
#define INITIAL_RECORD   (-1)


/* structure defines */
struct best_record_t {
    int level; /*!< the game level of current record */
    unsigned int second; /*!< the value presentation of time */
    /*!< the string presentation of current level */
    char level_str[LEVEL_STRING_LENGTH + 1];
    /*!< the string presentation of time */
    char time_str[TIME_STRING_LENGTH + 1];
    struct best_record_t *next; /*!< the next best record */
};

struct game_session_t {
    int player_row; /*!< the relative row of player inside the maze */
    int player_col; /*!< the relative column of player inside the maze */
    int virus_row; /*!< the relative row of virus inside the maze */
    int virus_col; /*!< the relative column of virus inside the maze */
    unsigned int level; /*!< the current game level */
    struct best_record_t best_record; /*!< the best_record_t pointer */
    unsigned int score; /*!< the total score since game session has started */
    unsigned int attempt; /*!< the attempts of player have tryed to play */
    unsigned int session_start_time; /*!< the start time of entire game */
    unsigned int level_start_time; /*!< the start time of current level */
    unsigned int status; /*!< the current level */
};
/* defines for game_session_t.status */
#define TITLE                   (0)
#define INSTRUCTION_IN_TITLE    (1)
#define MAIN                    (2)
#define PAUSE_IN_MAIN           (3)
#define INSTRUCTION_IN_MAIN     (4)
#define CATCHED_IN_MAIN         (5)
#define PASS_IN_MAIN            (6)
#define CLEAR_IN_MAIN           (7)
#define CHEAT_IN_MAIN           (8)


/* function declarations */
/**
 * @brief Game initial function for the first running.
 *
 * the position of both player and virus is set to -1 indicating that they
 * remain in the original position in the map layout.
 *
 * @param status       the current status of user at.
 * @param game_session the pointer to the game_session_t variable.
 */
void init_game_session(int status, struct game_session_t *game_session);

/**
 * @brief Initial the record of game.
 *
 * It uses link list to create MAZE_RECENT_RECORDS_NUM items with a first
 * dumming head(NULL item).
 *
 * @param game_session the pointer to the game_session_t variable.
 */
void init_records(struct game_session_t *game_session);

/**
 * @brief Display the title window of game.
 *
 * It wait for user's key press to start game.
 *
 * @param game_session the pointer to the game_session_t variable.
 */
void game_title(struct game_session_t *game_session);

/**
 * @brief Main function for running game.
 *
 * It receives key press to running the game and print different game UI.
 *
 * @param game_session the pointer to the game_session_t variable.
 */
void game_main(struct game_session_t *game_session);

/**
 * @brief Display the main interface of game.
 * @param game_session the pointer to the game_session_t variable.
 */
void display_main_layout(struct game_session_t *game_session);

/**
 * @brief Update timer.
 * @param start_time the start time of current level.
 */
void refresh_time(int start_time);

/**
 * @brief convert second value to string representation.
 * @param time_string string pointer.
 * @param time second value.
 */
void second_to_time_str(char time_string[TIME_STRING_LENGTH + 1],
                        unsigned int time);

/**
 * @brief move the player in the maze if valid.
 * @param  direction    up, down, left or right.
 * @param  game_session the pointer to the game_session_t variable.
 * @return              0 if current move can be performed, -1 if cannot,
 *                      1 if player has moved to destination.
 */
int move_player(char direction, struct game_session_t *game_session);

/**
 * @brief move the virus in the maze 2 steps if it can get closer to the player.
 * @param  game_session the pointer to the game_session_t variable.
 * @return              0 for not catching up player, 1 for catching up player.
 */
int move_virus(struct game_session_t *game_session);

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
                  unsigned int expected_row, unsigned int expected_col);

/**
 * @brief Set game_session for next new game level.
 * @param game_session [description]
 */
void set_to_next_level(struct game_session_t *game_session);

/**
 * Replace the old record with new one.
 * @param level       the game level to be replaced.
 * @param total_time  total time for passing that level.
 * @param best_record pointer to best_record variable.
 */
void replace_new_record(int level, int total_time,
                        struct best_record_t *best_record);
#endif /* H_GAME */
