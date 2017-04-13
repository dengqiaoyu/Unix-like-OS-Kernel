#ifndef __H_SCHEDULER__
#define __H_SCHEDULER__

#include <mutex.h>

#include "list.h"
#include "task.h"

#define SCHE_NODE_TO_TABLE_NODE(sche_node)\
        ((tcb_tb_node_t *)((char *)sche_node + 8))
#define SCHE_NODE_TO_TCB(sche_node)\
        ((thread_t *)((char *)sche_node + 24))
#define TCB_TO_SCHE_NODE(tcb_ptr)\
        ((sche_node_t *)((char *)tcb_ptr - 24))

typedef struct schedule_list_struct {
    list_t *active_list;
    list_t *sleeping_list;
} schedule_t;

typedef node_t sche_node_t;
typedef node_t tcb_tb_node_t;

int scheduler_init();

void set_cur_run_thread(thread_t *tcb_ptr);

void sche_yield(int status);

thread_t *get_cur_tcb();

void sche_push_back(thread_t *tcb_ptr);

void sche_push_front(thread_t *tcb_ptr);

void tranquilize(sleep_node_t *sleep_node);

#endif
