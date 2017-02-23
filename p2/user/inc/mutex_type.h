/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#define INIT_MUTEX {.lock = 0, .holder_tid = -1}

typedef struct mutex {
  int lock;
  int holder_tid;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
