/** @file flu_db.h
 *
 *  @brief Definitions for maze type
 *
 *  "Escape the Flu Virus" is similar to "Theseus and the
 *  Minotaur", invented by Robert Abbott and described in
 *  the April 1994 issue of "Games & Puzzles", as documented
 *  here:
 *  http://web.archive.org/web/20160711105035/http://www.logicmazes.com/theseus4.html
 *  "Escape the Flu Virus" is not endorsed in any way by
 *  Robert Abbott.
 *
 *  @author Tim Becker (tjbecker)
 */
#ifndef __FLU_DB_H_
#define __FLU_DB_H_

#define MAZE_MAX_HEIGHT 23
#define MAZE_MAX_WIDTH 42 

#define MAZE_ITEM_EXIT   'H' // University Health Services
#define MAZE_ITEM_PLAYER '@'
#define MAZE_ITEM_VIRUS  'V'

/** @brief A representation of a maze
 *
 *  @var origin A string indicating the author/source of the maze
 *  @var width The width of each row (<= MAZE_MAX_WIDTH)
 *  @var height The height of each column (<= MAZE_MAX_HEIGHT)
 *  @var maze An array of rows (strings).
 *
 *  Each even-numbered row (starting with the row at index 0) describes
 *  horizontal barriers.  In these rows:
 *  '-' is a horiontal wall. These may appear only at odd indices.
 *  ' ' indicates that the space may be freely moved *through*, but not *to*.
 *      These may appear only at even indices.
 *  '+' is the edge of a wall. These may appear only at even indices.
 *
 *  Each odd-numbered row (starting with the row at index 1) describes
 *  vertical barriers and playable spaces.  In these rows:
 *  '|' is a vertical wall.  These may appear only at even indices in the row.
 *  ' ' indicates that the space may be freely moved *through*, but not *to*.
 *      These may appear only at even indices.
 *  '.' is a free space which may be moved *to*. These may appear only at
 *      odd indices.
 *  '@', 'V', 'E' are the player, the flu virus, and the exit respectively.
 *      These may appear only at odd indices in the row.
 */
typedef struct flu_maze {
    char *origin;
    unsigned width;
    unsigned height;
    const char *maze[MAZE_MAX_HEIGHT];
} flu_maze_t;

extern const flu_maze_t flu_mazes[];
extern const int flu_maze_count;

#endif /* __FLU_DB_H_ */
