/** @file thread.c
 *
 */

#include <stdlib.h>
#include <types.h>
#include <mutex.h>
#include "thr_internals.h"
#include "tid_table.h"

#define MAX_THREADS 4096

unsigned int stack_size;
struct tid_table *table;

int thr_init( unsigned int size )
{
  stack_size = size;
  table = (struct tid_table *)calloc(1, sizeof(struct tid_table));
  if (table == NULL) return -1;
  return 0;
}

int thr_create( void *(*func)(void *), void *args )
{
  void *new_stack = malloc(stack_size);
  if (new_stack == NULL) return -1;
  new_stack = (void *)((char *)new_stack + stack_size);

  int tid = start_thread(new_stack, func, args);
  return tid;
}

int thr_join( int tid, void **statusp )
{
  return 0;
}

void thr_exit( void *status )
{
  return;
}

int thr_getid( void )
{
  return 0;
}

int thr_yield( int tid )
{
  return 0;
}

