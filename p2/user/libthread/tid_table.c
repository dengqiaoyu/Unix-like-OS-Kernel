/** @file tid_table.c
 *
 */

#include <stdlib.h>
#include <mutex.h>
#include "tid_table.h"

struct node {
  unsigned int tid;
  void *stackaddr;
  int exited;
  void *status;
  mutex_t lock;
  struct node *next;
};

int insert(struct tid_table *table, unsigned int tid, void *stackaddr)
{
  struct node *temp;

  // impossible
  if (table == NULL) return 1;

  temp = (struct node *)calloc(1, sizeof(struct node));

  // failed malloc
  if (temp == NULL) return 1;

  temp->tid = tid;
  temp->stackaddr = stackaddr;

  if (table->head == NULL) {
    table->head = temp;
    temp->next = NULL;
  }
  else {
    temp->next = table->head;
    table->head = temp;
  }
  return 0;
}

void *remove(struct tid_table *table, unsigned int tid)
{
  struct node *temp, *prev;
  void *status;

  // impossible
  if (table == NULL) return (void *)0;

  temp = table->head;
  while (temp != NULL) {
    if (temp->tid == tid) {
      if (temp == table->head) table->head = temp->next;
      else prev->next = temp->next;
      status = temp->status;
      free(temp);
      return status;
    }
    else {
      prev = temp;
      temp = temp->next;
    }
  }
  // impossible
  return (void *)0;
}

