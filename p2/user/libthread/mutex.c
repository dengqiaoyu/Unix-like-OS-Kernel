/** @file mutex.c
 *  @brief
 */

#include <mutex.h>

int mutex_init(mutex_t *mp)
{
  mp->lock = 0;
  return 0;
}

void mutex_destroy(mutex_t *mp)
{
  return;
}

