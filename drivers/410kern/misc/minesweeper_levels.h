/** @file minesweeper_levels.h
 *  @brief Level description prototypes
 *
 *  @author Michael Cui (mcui)
 *  @bug No known bugs.
 */

#ifndef _LEVELS_H_
#define _LEVELS_H_

#define MAX_LEVELS 32

// a struct containing information about one level in minesweeper 
// The actual level map is stored a string in the member variable
// map.  
// If you look at the structures defined in levels.c you can see 
// that mines are stored as 'X', and non-mines are stored as 
// digits '0' to '8'

typedef struct {
  int width;
  int height;
  int num_mines;
  char *map;
} level_t;

const level_t *levels[MAX_LEVELS];
const int numlevels;

#endif /* _LEVELS_H_ */
