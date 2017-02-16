/** @file tid_table.h
 *
 */

#ifndef _TID_TABLE_H_
#define _TID_TABLE_H_

struct tid_table {
  struct node *head;
};

int insert(struct tid_table *table, unsigned int tid, void *stackaddr);
void *remove(struct tid_table *table, unsigned int tid);

#endif
