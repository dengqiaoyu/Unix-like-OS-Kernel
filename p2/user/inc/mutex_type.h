/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#define INIT_MUTEX {.lock = 0}

typedef struct mutex {
  /* fill this in */
  int lock;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
