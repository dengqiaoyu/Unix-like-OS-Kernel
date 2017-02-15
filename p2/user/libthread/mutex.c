/** @file mutex.c
 *  @brief
 */

#include <mutex.h>

int mutex_init( mutex_t *mp )
{
  mp->init = 1;
  return 0;
}

void mutex_destroy( mutex_t *mp )
{
  mp->init = 0;
  return;
}

void mutex_lock( mutex_t *mp )
{
  return;
}

void mutex_unlock( mutex_t *mp )
{
  return;
}

